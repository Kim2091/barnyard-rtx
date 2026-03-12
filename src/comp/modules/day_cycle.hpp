#pragma once

namespace comp
{
	class day_cycle final : public shared::common::loader::component_module
	{
	public:
		day_cycle();
		~day_cycle() = default;

		static inline day_cycle* p_this = nullptr;
		static day_cycle* get() { return p_this; }

		static bool is_initialized()
		{
			if (const auto mod = get(); mod && mod->m_initialized) {
				return true;
			}
			return false;
		}

		void update();
		void draw_sun_light();

		float get_day_time() const { return m_day_time; }
		const Vector& get_sun_direction() const { return m_sun_direction; }
		const Vector& get_moon_direction() const { return m_moon_direction; }
		float get_sun_elevation() const { return m_sun_elevation; }

	private:
		static constexpr float DAY_CYCLE_RANGE = 1200.0f;
		static constexpr uintptr_t GAME_TIME_MANAGER_PTR = 0x00783d3c;
		static constexpr uintptr_t SKY_OBJECT_PTR = 0x0079b328;

		float read_day_time();
		void compute_celestial_directions(float day_time);
		void update_remix_light();
		void override_sky_object();

		bool m_initialized = false;
		bool m_cleared_light_converter = false;

		float m_day_time = 0.0f;
		Vector m_sun_direction = { 0.0f, 0.0f, 1.0f };
		Vector m_moon_direction = { 0.0f, 0.0f, -1.0f };
		float m_sun_elevation = 0.0f;
		float m_sun_intensity = 0.0f;

		remixapi_LightHandle m_sun_light_handle = nullptr;
	};
}
