#include "std_include.hpp"
#include "day_cycle.hpp"

#include "comp_settings.hpp"
#include "renderer.hpp"
#include "shared/common/remix_api.hpp"

namespace comp
{
	float day_cycle::read_day_time()
	{
		auto** pp_mgr = reinterpret_cast<game::AGameTimeManager**>(GAME_TIME_MANAGER_PTR);
		if (!pp_mgr || !*pp_mgr) {
			return -1.0f;
		}
		return (*pp_mgr)->m_flDayTime;
	}

	// Map game time [0, 1200) to sun and moon directions.
	// frac 0.0 = dawn, 0.25 = noon, 0.5 = dusk, 0.75 = midnight.
	void day_cycle::compute_celestial_directions(float day_time)
	{
		auto cs = comp_settings::get();
		const float time_offset = cs->day_cycle_time_offset._float();
		const float tilt_deg = cs->day_cycle_tilt._float();

		const float adjusted = fmodf(day_time + time_offset, DAY_CYCLE_RANGE);
		const float frac = adjusted / DAY_CYCLE_RANGE;
		const float angle = frac * 2.0f * static_cast<float>(M_PI);
		const float tilt_rad = DEG2RAD(tilt_deg);

		float sx = cosf(angle);
		float sy = sinf(angle) * sinf(tilt_rad);
		float sz = sinf(angle) * cosf(tilt_rad);

		m_sun_direction = Vector(sx, sy, sz);
		m_sun_direction.NormalizeChecked();
		m_sun_elevation = sz;

		// Moon is opposite the sun
		m_moon_direction = -m_sun_direction;

		// Intensity via smoothstep on elevation
		const float t = std::clamp((m_sun_elevation + 0.05f) / 0.25f, 0.0f, 1.0f);
		m_sun_intensity = t * t * (3.0f - 2.0f * t);
	}

	void day_cycle::ensure_light_converter_cleared()
	{
		auto& api = shared::common::remix_api::get();
		if (!m_cleared_light_converter && api.is_initialized() && api.m_bridge.SetConfigVariable)
		{
			api.m_bridge.SetConfigVariable("rtx.lightConverter", "");
			shared::common::log("DayCycle", "Cleared rtx.lightConverter",
				shared::common::LOG_TYPE::LOG_TYPE_STATUS, true);
			m_cleared_light_converter = true;
		}
	}

	// Write our computed sun/moon direction into the game's sky object so the
	// billboard sprites track the Remix distant light.
	//
	// Sky object layout (verified via dynamic analysis):
	//   +0x00  float[4]  current interpolated direction (game writes each frame)
	//   +0x10  float[4]  moon/corona direction (static, used by billboard placement)
	//   +0x20  float[4]  night interpolation endpoint
	//   +0x30  float[4]  day interpolation endpoint
	//
	// The game lerps [0x00] from [0x20] and [0x30] based on time of day.
	// By writing both endpoints, the interpolation always yields our direction.
	void day_cycle::override_sky_object()
	{
		auto** pp_sky = reinterpret_cast<uint8_t**>(SKY_OBJECT_PTR);
		if (!pp_sky || !*pp_sky) {
			return;
		}

		uint8_t* sky = *pp_sky;

		auto write_dir = [](float* dst, const Vector& dir) {
			dst[0] = dir.x;
			dst[1] = dir.y;
			dst[2] = dir.z;
			dst[3] = 0.0f;
		};

		// Override both interpolation endpoints so the game's lerp
		// always produces our computed sun direction at +0x00
		write_dir(reinterpret_cast<float*>(sky + 0x20), m_sun_direction);
		write_dir(reinterpret_cast<float*>(sky + 0x30), m_sun_direction);

		// Moon direction is static at +0x10
		write_dir(reinterpret_cast<float*>(sky + 0x10), m_moon_direction);
	}

	// Called from the Remix begin-scene callback -- the latest point before
	// Remix composites the frame.  We create and draw the light here so the
	// camera-space transform uses the freshest camera matrix possible,
	// eliminating jitter from frame-to-frame camera lag.
	void day_cycle::draw_sun_light()
	{
		auto& api = shared::common::remix_api::get();
		if (!api.is_initialized()) {
			return;
		}

		auto cs = comp_settings::get();
		if (!cs->day_cycle_enabled._bool()) {
			return;
		}

		const float base_intensity = cs->day_cycle_sun_intensity._float() * m_sun_intensity;

		if (m_sun_light_handle) {
			api.m_bridge.DestroyLight(m_sun_light_handle);
			m_sun_light_handle = nullptr;
		}

		if (base_intensity <= 0.001f) {
			return;
		}

		// Transform world-space sun direction into view/camera space.
		// Remix reads the D3D9 view matrix which this game sets to identity,
		// so it treats our direction as camera-relative.  Compensate by
		// dotting with the camera's world-space axes (= multiply by the
		// transpose of the upper-left 3x3 of the camera world matrix).
		const auto& cm = g_current_camera_mtx;
		Vector dir;
		dir.x = m_sun_direction.x * cm.m[0][0] + m_sun_direction.y * cm.m[0][1] + m_sun_direction.z * cm.m[0][2];
		dir.y = m_sun_direction.x * cm.m[1][0] + m_sun_direction.y * cm.m[1][1] + m_sun_direction.z * cm.m[1][2];
		dir.z = m_sun_direction.x * cm.m[2][0] + m_sun_direction.y * cm.m[2][1] + m_sun_direction.z * cm.m[2][2];
		dir.NormalizeChecked();

		const Vector base_color(
			cs->day_cycle_sun_color._vec_x(),
			cs->day_cycle_sun_color._vec_y(),
			cs->day_cycle_sun_color._vec_z());

		const float warmth = 1.0f - std::clamp(m_sun_elevation, 0.0f, 1.0f);
		const Vector tint(1.0f, 1.0f - warmth * 0.3f, 1.0f - warmth * 0.5f);
		const Vector final_color = base_color * tint;

		remixapi_LightInfoDistantEXT ext = {};
		ext.sType = REMIXAPI_STRUCT_TYPE_LIGHT_INFO_DISTANT_EXT;
		ext.pNext = nullptr;
		ext.direction = dir.ToRemixFloat3D();
		ext.angularDiameterDegrees = cs->day_cycle_sun_angular_diameter._float();
		ext.volumetricRadianceScale = 1.0f;

		remixapi_LightInfo info = {};
		info.sType = REMIXAPI_STRUCT_TYPE_LIGHT_INFO;
		info.pNext = &ext;
		info.hash = shared::utils::string_hash64("sun_distant_light");
		info.radiance = {
			final_color.x * base_intensity,
			final_color.y * base_intensity,
			final_color.z * base_intensity
		};

		api.m_bridge.CreateLight(&info, &m_sun_light_handle);
		api.m_bridge.DrawLightInstance(m_sun_light_handle);
	}

	void day_cycle::update()
	{
		auto cs = comp_settings::get();
		if (!cs->day_cycle_enabled._bool()) {
			return;
		}

		const float day_time = read_day_time();
		if (day_time < 0.0f) {
			return;
		}

		m_day_time = day_time;
		compute_celestial_directions(day_time);
		ensure_light_converter_cleared();
		override_sky_object();
	}

	day_cycle::day_cycle()
	{
		p_this = this;
		m_initialized = true;
		shared::common::log("DayCycle", "Module initialized.", shared::common::LOG_TYPE::LOG_TYPE_DEFAULT, false);
	}
}
