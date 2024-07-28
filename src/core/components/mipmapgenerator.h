#pragma once
#include <vector>
#include <string>
#include "pso.h"
#include "rootsignature.h"
#include "resources/colorbuffer.h"


class MipmapGenerator
{
public:
	enum class MipmapType
	{
		XYEVEN = 0,
		XODD = 1,
		YODD = 2,
		XYODD= 3,
	};


public:
	void Initialize();

	void GenerateMipmap(ColorBuffer* colorbuffer);


private:
	void InitializeRS();
	void InitializePSO();

private:
	RootSignature m_rootSig;
	ComputePSO m_psos[4];
	

};