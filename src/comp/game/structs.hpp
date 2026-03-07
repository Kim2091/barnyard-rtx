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
}
