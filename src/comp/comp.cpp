#include "std_include.hpp"

#include "modules/imgui.hpp"
#include "modules/renderer.hpp"
#include "shared/common/dinput_hook_v2.hpp"
#include "shared/common/remix_api.hpp"

// see comment in main()
//#include "shared/common/dinput_hook_v1.hpp"
//#include "shared/common/dinput_hook_v2.hpp"

namespace comp
{
	void on_begin_scene_cb()
	{
		if (!tex_addons::initialized) {
			tex_addons::init_texture_addons();
		}

		// fake camera

		const auto& im = imgui::get();
		if (im->m_dbg_use_fake_camera)
		{
			D3DXMATRIX view_matrix
			(
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 0.447f, 0.894f, 0.0f,
				0.0f, -0.894f, 0.447f, 0.0f,
				0.0f, 100.0f, -50.0f, 1.0f
			);

			D3DXMATRIX proj_matrix
			(
				1.359f, 0.0f, 0.0f, 0.0f,
				0.0f, 2.414f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.001f, 1.0f,
				0.0f, 0.0f, -1.0f, 0.0f
			);

			// Construct view matrix
			D3DXMATRIX rotation, translation;
			D3DXMatrixRotationYawPitchRoll(&rotation,
				D3DXToRadian(im->m_dbg_camera_yaw),		// Yaw in radians
				D3DXToRadian(im->m_dbg_camera_pitch),	// Pitch in radians
				0.0f);									// No roll for simplicity

			D3DXMatrixTranslation(&translation,
				-im->m_dbg_camera_pos[0], // Negate for camera (moves world opposite)
				-im->m_dbg_camera_pos[1],
				-im->m_dbg_camera_pos[2]);

			D3DXMatrixMultiply(&view_matrix, &rotation, &translation);

			// Construct projection matrix
			D3DXMatrixPerspectiveFovLH(&proj_matrix,
				D3DXToRadian(im->m_dbg_camera_fov), // FOV in radians
				im->m_dbg_camera_aspect,
				im->m_dbg_camera_near_plane,
				im->m_dbg_camera_far_plane);

			shared::globals::d3d_device->SetTransform(D3DTS_WORLD, &shared::globals::IDENTITY);
			shared::globals::d3d_device->SetTransform(D3DTS_VIEW, &view_matrix);
			shared::globals::d3d_device->SetTransform(D3DTS_PROJECTION, &proj_matrix);
		}

		// Actual camera setup here if matrices are available
		else 
		{
			shared::globals::d3d_device->SetTransform(D3DTS_WORLD, &shared::globals::IDENTITY); // does not hurt
			shared::globals::d3d_device->SetTransform(D3DTS_VIEW, &shared::globals::IDENTITY); // always identity

			// Example code if you managed to find some kind of matrix struct
				//if (const auto viewport = game::vp; viewport)
				//{
				//	shared::globals::d3d_device->SetTransform(D3DTS_VIEW, &viewport->view);
				//	shared::globals::d3d_device->SetTransform(D3DTS_PROJECTION, &viewport->proj);
				//}
		}

		struct cam
		{
			int pad;
			D3DXMATRIX m_Matrix;
			float m_fFOV;
			float m_fProjectionCentreX;
			float m_fProjectionCentreY;
		};

		DWORD camera_manager_ptr = *(DWORD*)0x7822E0;
		if (camera_manager_ptr)
		{
			DWORD* camera_manager = reinterpret_cast<DWORD*>(camera_manager_ptr);
			const auto current_camera = shared::utils::hook::call<cam * (__fastcall)(DWORD * this_ptr, void* thiscall_arg)>(0x45B870)(camera_manager, nullptr);

			if (current_camera) 
			{
				g_current_camera_mtx = current_camera->m_Matrix;
				g_current_camera_origin.x = current_camera->m_Matrix.m[3][0];
				g_current_camera_origin.y = current_camera->m_Matrix.m[3][1];
				g_current_camera_origin.z = current_camera->m_Matrix.m[3][2];
			}
		}
	}

	// force render objects close to the camera
	bool __stdcall cull_sphere_to_frustum_simple(const game::tsphere* a_rSphere, const game::tplane* a_pPlanes, int a_iNumPlanes)
	{
		// force render if close to camera
		const float dist_sq = (a_rSphere->m_Origin - g_current_camera_origin).LengthSqr();

		const float r = imgui::get()->m_debug_vector.x + a_rSphere->m_fRadius; // 15
		if (dist_sq <= r * r) {
			return true;
		}

		for (auto i = 0u; i < 6u; i++)
		{
			float dist = a_rSphere->m_Origin.Dot(a_pPlanes[i].m_Normal);
			if (a_rSphere->m_fRadius < dist - a_pPlanes[i].m_fDistance)
				return false;
		}

		return true;
	}

	// ---

	int render_tree_intersect_hk(const game::tsphere* a_rSphere)
	{
		// force render if close to camera
		const float dist_sq = (a_rSphere->m_Origin - g_current_camera_origin).LengthSqr();

		const float r = imgui::get()->m_debug_vector.y /*+ a_rSphere->m_fRadius*/; // 50
		if (dist_sq <= r * r) {
			return 1;
		}

		return 0;
	}

	__declspec (naked) void render_tree_intersect_stub()
	{
		static uint32_t func_addr = 0x6D4DD0; // Frustum::IntersectSphereReduce
		static uint32_t jmp_addr  = 0x6D552F; // draw node
		static uint32_t retn_addr = 0x6D54FB; // continued checks
		__asm
		{
			call	func_addr; // og
			sub		eax, 0; // og - same as cmp
			je		DRAW;

			// would normally be culled or re-checked from here on
			push	ecx; // save
			lea     ecx, [esp + 0x18 + 4]; // sphere .. +4 because of ecx push
			pushad;
			push	ecx; // sphere
			call	render_tree_intersect_hk;
			add		esp, 4;

			cmp		eax, 1;
			je		DRAW_POPAD; // nearby -> forced

			popad;
			pop		ecx; // restore
			jmp		retn_addr; // let game continue checks

		DRAW_POPAD:
			popad;
			pop		ecx; // restore

		DRAW:
			jmp		jmp_addr;

			//jmp		retn_addr;
		}
	}

	void main()
	{
		// #Step 2: init remix api if you want to use it or comment it otherwise
		// Requires "exposeRemixApi = True" in the "bridge.conf" that is located in the .trex folder
		shared::common::remix_api::initialize(nullptr, nullptr, nullptr, false);

		// init modules which do not need to be initialized, before the game inits, here
		shared::common::loader::module_loader::register_module(std::make_unique<imgui>());
		shared::common::loader::module_loader::register_module(std::make_unique<renderer>());

		// #Step 3: hook dinput if your game uses direct input (for ImGui) - ONLY USE ONE
		//shared::common::loader::module_loader::register_module(std::make_unique<shared::common::dinput_v1>()); // v1: might cause issues with the Alt+X menu
		shared::common::loader::module_loader::register_module(std::make_unique<shared::common::dinput_v2>()); // v2: better but might need further tweaks

		// ----------

		// 
		shared::utils::hook(0x6D54F1, render_tree_intersect_stub, HOOK_JUMP).install()->quick();

		// force render objects close to the camera
		shared::utils::hook(0x6CEAD0, cull_sphere_to_frustum_simple, HOOK_JUMP).install()->quick();

		MH_EnableHook(MH_ALL_HOOKS);
	}
}
