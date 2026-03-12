#pragma once

namespace comp::game
{
	// place any game structures here

	struct some_struct_containing_matrices
	{
		D3DXMATRIX view;
		D3DXMATRIX proj;
	};

	struct tsphere
	{
		Vector m_Origin;
		float m_fRadius;
	};

	struct tplane
	{
		Vector m_Normal;
		float m_fDistance;
	};

	// Global pointer at 0x00783d3c: AGameTimeManager** g_ppGameTimeManager
	struct AGameTimeManager
	{
		char  PADDING[0x34];
		int32_t m_eDayPhase;  // 0x34
		float m_flDayTime;    // 0x38  range [0, 1200)
	};
}
