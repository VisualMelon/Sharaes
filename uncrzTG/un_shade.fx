struct VS_Input
{
	float4 pos : POSITION0;
	float4 nrm : NORMAL0;
	float4 col : COLOR0;
	float tti : TEXCOORD0;
};

struct VS_Output
{
	float4 pos : POSITION0;
	float4 nrm : TEXCOORD2;
	float4 altPos : TEXCOORD1;
	float4 col : COLOR0;
};

struct VS_Input_Tex
{
	float4 pos : POSITION0;
	float4 nrm : NORMAL0;
	float4 col : COLOR0;
	float2 txc : TEXCOORD0;
	float tti : TEXCOORD1;
};

struct VS_Output_Tex
{
	float4 pos : POSITION0;
	float lit : TEXCOORD3;
	float4 altPos : TEXCOORD1;
	float4 col : COLOR0;
	float2 txc : TEXCOORD0;
	float4 lmc: TEXCOORD2;
};

struct VS_Input_Decal
{
	float4 pos : POSITION0;
	float4 nrm : NORMAL0;
	float col : COLOR0;
	float4 txc : TEXCOORD0;
	float tti : TEXCOORD1;
};

struct VS_Output_Decal
{
	float4 pos : POSITION0;
	float lit : TEXCOORD3;
	float4 altPos : TEXCOORD1;
	float col : COLOR0;
	float4 txc : TEXCOORD0;
	float4 lmc: TEXCOORD2;
};

struct VS_Output_Dyn
{
	float4 pos : POSITION0;
	float4 altPos : TEXCOORD0;
	float4 lmc: TEXCOORD1;
};

struct VS_Input_Terrain
{
	float4 pos : POSITION0;
	float4 nrm : NORMAL0;
	float2 txc : TEXCOORD0;
	float4 w : TEXCOORD1;
};

struct VS_Output_Terrain
{
	float4 pos : POSITION0;
	float lit : TEXCOORD5;
	float2 txc : TEXCOORD0;
	float4 w : TEXCOORD1;
	float4 altPos : TEXCOORD2;
	float orgY : TEXCOORD3;
	float4 lmc: TEXCOORD4;
};

struct VS_Output_Light
{
	float4 pos : POSITION0;
	float4 altPos : TEXCOORD0;
};

struct VS_Input_Over
{
	float4 pos : POSITION0;
	float2 txc : TEXCOORD0;
};

struct VS_Output_Over
{
	float4 pos : POSITION0;
	float2 txc : TEXCOORD0;
	float4 altPos : TEXCOORD1;
};

struct PS_Output
{
	float4 col : COLOR0;
};

float4x4 viewMat;
float4x4 projMat;
float4x4 viewProj; // viewMat * viewProj... or the other way round, I don't really know
float4x4 lightViewProj;
float4 eyePos;
float4 eyeDir;

Texture tex;
sampler texSampler = sampler_state { texture = <tex>;magfilter = NONE; minfilter = NONE; mipfilter = NONE; AddressU = mirror; AddressV = mirror;}; // no linear 
sampler texBorderSampler = sampler_state { texture = <tex>;magfilter = NONE; minfilter = NONE; mipfilter = NONE; AddressU = border; AddressV = border; BorderColor = 0x00000000;}; // border
sampler texLinearSampler = sampler_state { texture = <tex>;magfilter = LINEAR; minfilter = LINEAR; mipfilter = LINEAR; AddressU = mirror; AddressV = mirror;}; // linear 
float4 texData; // (offX, offY,) mulX, mulY

float4x4 vpMat;
float4 targTexData; // offX, offY, mulX, mulY
Texture targTex;
sampler targTexSampler = sampler_state { texture = <targTex>;magfilter = NONE; minfilter = NONE; mipfilter = NONE; AddressU = mirror; AddressV = mirror;};

float lightCoof;
float lightConeness;
float lightDepth = 100;
float4 lightPos;
float4 lightDir;
float lightDodge = 0.001;
float lightType;
float4 lightAmbient;
float4 lightColMod;

Texture lightTex;
sampler lightTexSampler = sampler_state { texture = <lightTex>;magfilter = LINEAR; minfilter = LINEAR; mipfilter = LINEAR; AddressU = border; AddressV = border; BorderColor = 0xFFFFFFFF;};
Texture lightPatternTex;
sampler lightPatternTexSampler = sampler_state { texture = <lightPatternTex>;magfilter = NONE; minfilter = NONE; mipfilter = NONE; AddressU = border; AddressV = border; BorderColor = 0x00000000;};
sampler lightPatternTexLinearSampler = sampler_state { texture = <lightPatternTex>;magfilter = LINEAR; minfilter = LINEAR; mipfilter = LINEAR; AddressU = border; AddressV = border; BorderColor = 0x00000000;};

Texture tex0;
sampler tex0Sampler = sampler_state { texture = <tex0>;magfilter = NONE; minfilter = NONE; mipfilter = NONE; AddressU = mirror; AddressV = mirror;};
Texture tex1;
sampler tex1Sampler = sampler_state { texture = <tex1>;magfilter = NONE; minfilter = NONE; mipfilter = NONE; AddressU = mirror; AddressV = mirror;};
Texture tex2;
sampler tex2Sampler = sampler_state { texture = <tex2>;magfilter = NONE; minfilter = NONE; mipfilter = NONE; AddressU = mirror; AddressV = mirror;};
Texture tex3;
sampler tex3Sampler = sampler_state { texture = <tex3>;magfilter = NONE; minfilter = NONE; mipfilter = NONE; AddressU = mirror; AddressV = mirror;};

float4x4 transarr[30]; // need to be same as number of segs
float4 spriteLoc[100]; // sprite buffer size must be no more than (this len / size of sprite data)
float4 spriteDim;
float4 colMod;
float farDepth = 1000;
float invFarDepth = 0.001;

float ticker = 0;

float clampPositive(float num)
{
	if (num < 0)
		return 0;
	return num;
}

float4 lightTransOrtho(float4 pos)
{
	float4 res = mul(pos, lightViewProj);
	return res;
}

float4 lightTransPersp(float4 pos)
{
	float4 res = mul(pos, lightViewProj);
	res.z = res.z * res.w / lightDepth;
	return res;
}

float4 lightTransPoint(float4 pos)
{
	// this should never be called
	return pos;
}

float4 lightTrans(float4 pos)
{
	if (lightType == 0)
		return lightTransOrtho(pos);
	else if (lightType == 1)
		return lightTransPersp(pos);
	else if (lightType == 2)
		return lightTransPoint(pos);
	return (float4)0;
}

float4 lightTransOrthoVP(float4 pos)
{
	float4 res = mul(pos, viewProj);
	return res;
}

float4 lightTransPerspVP(float4 pos)
{
	float4 res = mul(pos, viewProj);
	res.z = res.z * res.w / lightDepth;
	return res;
}

float4 lightTransPointVP(float4 pos)
{
	// this should never be called
	return pos;
}

float4 lightTransVP(float4 pos)
{
	if (lightType == 0)
		return lightTransOrthoVP(pos);
	else if (lightType == 1)
		return lightTransPerspVP(pos);
	else if (lightType == 2)
		return lightTransPointVP(pos);
	return (float4)0;
}

float4 lightUnTransOrtho(float4 pos)
{
	float4 res = pos;
	return res;
}

float4 lightUnTransPersp(float4 pos)
{
	float4 res = pos;
	res.x = res.x / res.w;
	res.y = res.y / res.w;
	res.z = res.z / res.w;
	return res;
}

float4 lightUnTransPoint(float4 pos)
{
	return pos;
}

float4 lightUnTrans(float4 pos)
{
	if (lightType == 0)
		return lightUnTransOrtho(pos);
	else if (lightType == 1)
		return lightUnTransPersp(pos);
	else if (lightType == 2)
		return lightUnTransPoint(pos);
	return (float4)0;
}

float lightLitnessOrtho(float4 pos, float4 nrm)
{
	return -dot(nrm, lightDir);
}

float lightLitnessPersp(float4 pos, float4 nrm)
{
	float4 plDir = pos - lightPos;
	float pldRMod = rsqrt(plDir.x * plDir.x + plDir.y * plDir.y + plDir.z * plDir.z);
	plDir *= pldRMod;
	return -dot(nrm, plDir);
}

float lightLitnessPoint(float4 pos, float4 nrm)
{
	float4 plDir = pos - lightPos;
	float pldRMod = rsqrt(plDir.x * plDir.x + plDir.y * plDir.y + plDir.z * plDir.z);
	plDir *= pldRMod;
	return -dot(nrm, plDir);
}

float lightLitness(float4 pos, float4 nrm)
{
	if (lightType == 0)
		return lightLitnessOrtho(pos, nrm);
	else if (lightType == 1)
		return lightLitnessPersp(pos, nrm);
	else if (lightType == 2)
		return lightLitnessPoint(pos, nrm);
	return 0;
}

float4 calcLightModOrtho(float4 lmc)
{
	lmc = lightUnTransOrtho(lmc);

	float4 lightMod = 0.0;

	float2 lightCoords;
	lightCoords.x = lmc.x;
	lightCoords.y = lmc.y;

	float targDist = lmc.z;

	float4 lightCol = tex2D(lightTexSampler, lightCoords);
	float lightDist = lightCol.x;

	if (lightDodge + lightDist > targDist)
	{
		lightMod = tex2D(lightPatternTexSampler, lightCoords) * lightColMod;
		return lightMod;
	}
	lightMod = 0;
	return lightMod;
}
// these two (calcLightModOrtho and calcLightModPersp) are the SAME ATM
float4 calcLightModPersp(float4 lmc)
{
	lmc = lightUnTransPersp(lmc);

	float4 lightMod = 0.0;

	float2 lightCoords;
	lightCoords.x = lmc.x;
	lightCoords.y = lmc.y;

	float targDist = lmc.z;

	float4 lightCol = tex2D(lightTexSampler, lightCoords);
	float lightDist = lightCol.x;

	if (lightDodge + lightDist > targDist)
	{
		lightMod = tex2D(lightPatternTexSampler, lightCoords) * lightColMod;
		lightMod *= (1 - targDist * targDist);
		return lightMod;
	}
	lightMod = 0;
	return lightMod;
}

float4 calcLightModPoint(float4 lmc)
{
	// lmc is infact altPos in disguise
	lmc = lightUnTransPoint(lmc);

	float4 lightMod = 0.0;

	float x = lmc.x - lightPos.x;
	float y = lmc.y - lightPos.y;
	float z = lmc.z - lightPos.z;

	float targDist = (x * x + y * y + z * z);

	targDist = 1.0 - targDist / (lightDepth * lightDepth);
	clip(targDist);

	lightMod = lightColMod * targDist;
	return lightMod;
}

float4 calcLightMod(float4 lmc)
{
	if (lightType == 0)
		return calcLightModOrtho(lmc);
	else if (lightType == 1)
		return calcLightModPersp(lmc);
	else if (lightType == 2)
		return calcLightModPoint(lmc);
	return 0;
}

float4 calcDynModOrtho(float4 lmc)
{
	lmc = lightUnTransOrtho(lmc);

	float4 lightMod = 0.0;

	float2 lightCoords;
	lightCoords.x = lmc.x;
	lightCoords.y = lmc.y;

	lightMod = tex2D(lightPatternTexSampler, lightCoords) * lightColMod;

	return lightMod;
}

float4 calcDynModPersp(float4 lmc)
{
	lmc = lightUnTransPersp(lmc);

	float4 lightMod = 0.0;

	float2 lightCoords;
	lightCoords.x = lmc.x;
	lightCoords.y = lmc.y;

	lightMod = tex2D(lightPatternTexSampler, lightCoords) * lightColMod;

	return lightMod;
}

float4 calcDynMod(float4 lmc)
{
	if (lightType == 0)
		return calcDynModOrtho(lmc);
	else if (lightType == 1)
		return calcDynModPersp(lmc);
	return 0;
}




VS_Output VShade_Dull(VS_Input inp)
{
	VS_Output outp = (VS_Output)0;
	outp.pos = mul(inp.pos, viewProj);
	outp.col = inp.col;
	return outp;
}

PS_Output PShade_Dull(VS_Output inp)
{
	PS_Output outp = (PS_Output)0;
	outp.col = inp.col;
	return outp;
}



VS_Output_Tex VShade_Ui(VS_Input_Tex inp)
{
	VS_Output_Tex outp = (VS_Output_Tex)0;
	
	outp.pos = inp.pos;
	outp.pos.w = 1;
	outp.col = inp.col;
	outp.txc = inp.txc;

	return outp;
}

PS_Output PShade_Ui(VS_Output_Tex inp)
{
	PS_Output outp = (PS_Output)0;
	outp.col = inp.col * tex2D(texLinearSampler, inp.txc) * colMod;

	return outp;
}



VS_Output VShade(VS_Input inp)
{
	VS_Output outp = (VS_Output)0;
	if (inp.tti >= 0)
	{
		outp.pos = mul(mul(inp.pos, transarr[inp.tti]), viewProj);
	}
	else
	{
		outp.pos = mul(inp.pos, viewProj);
	}
	outp.col = inp.col;
	return outp;
}

PS_Output PShade(VS_Output inp)
{
	PS_Output outp = (PS_Output)0;
	outp.col = inp.col * colMod;

	return outp;
}

VS_Output_Tex VShade_Tex(VS_Input_Tex inp)
{
	VS_Output_Tex outp = (VS_Output_Tex)0;
	if (inp.tti >= 0)
	{
		inp.pos = mul(inp.pos, transarr[inp.tti]);
		inp.nrm = mul(inp.nrm, transarr[inp.tti]);
	}
	outp.pos = mul(inp.pos, viewProj);
	outp.altPos = outp.pos;
	outp.altPos.z = outp.altPos.z * outp.altPos.w * invFarDepth;
	outp.col = inp.col;
	outp.txc = inp.txc;

	return outp;
}

/// TEST CODE
VS_Output_Tex VShade_Tex_Test(VS_Input_Tex inp)
{
	VS_Output_Tex outp = (VS_Output_Tex)0;
	if (inp.tti >= 0)
	{
		inp.pos = mul(inp.pos, transarr[inp.tti]);
		inp.nrm = mul(inp.nrm, transarr[inp.tti]);
	}
	outp.pos = mul(inp.pos, viewProj);
	outp.altPos = outp.pos;
	outp.col = inp.col;
	outp.txc = inp.txc;

	return outp;
}
/// END TEST CODE

// VShade_Tex_Lit
VS_Output_Tex VShade_Tex_LitOrtho(VS_Input_Tex inp)
{
	VS_Output_Tex outp = (VS_Output_Tex)0;
	if (inp.tti >= 0)
	{
		inp.pos = mul(inp.pos, transarr[inp.tti]);
		inp.nrm = mul(inp.nrm, transarr[inp.tti]);
	}
	outp.pos = mul(inp.pos, viewProj);
	outp.altPos = outp.pos;
	outp.altPos.z = outp.altPos.z * outp.altPos.w * invFarDepth;
	outp.col = inp.col;
	outp.txc = inp.txc;

	// light
	outp.lit = lightLitnessOrtho(inp.pos, inp.nrm);
	outp.lmc = lightTransOrtho(inp.pos);

	return outp;
}

VS_Output_Tex VShade_Tex_LitPersp(VS_Input_Tex inp)
{
	VS_Output_Tex outp = (VS_Output_Tex)0;
	if (inp.tti >= 0)
	{
		inp.pos = mul(inp.pos, transarr[inp.tti]);
		inp.nrm = mul(inp.nrm, transarr[inp.tti]);
	}
	outp.pos = mul(inp.pos, viewProj);
	outp.altPos = outp.pos;
	outp.altPos.z = outp.altPos.z * outp.altPos.w * invFarDepth;
	outp.col = inp.col;
	outp.txc = inp.txc;

	// light
	outp.lit = lightLitnessPersp(inp.pos, inp.nrm);
	outp.lmc = lightTransPersp(inp.pos);

	return outp;
}

VS_Output_Tex VShade_Tex_LitPoint(VS_Input_Tex inp)
{
	VS_Output_Tex outp = (VS_Output_Tex)0;
	if (inp.tti >= 0)
	{
		inp.pos = mul(inp.pos, transarr[inp.tti]);
		inp.nrm = mul(inp.nrm, transarr[inp.tti]);
	}
	outp.pos = mul(inp.pos, viewProj);
	outp.altPos = outp.pos;
	outp.altPos.z = outp.altPos.z * outp.altPos.w * invFarDepth;
	outp.col = inp.col;
	outp.txc = inp.txc;

	// light
	outp.lit = lightLitnessPoint(inp.pos, inp.nrm);
	outp.lmc = lightTransPoint(inp.pos);

	return outp;
}



// VShade_Tex_Light
VS_Output_Light VShade_Tex_LightOrtho(VS_Input_Tex inp)
{
	VS_Output_Light outp = (VS_Output_Light)0;
	if (inp.tti >= 0)
	{
		outp.pos = lightTransOrthoVP(mul(inp.pos, transarr[inp.tti]));
	}
	else
	{
		outp.pos = lightTransOrthoVP(inp.pos);
	}
	outp.altPos = outp.pos;
	return outp;
}

VS_Output_Light VShade_Tex_LightPersp(VS_Input_Tex inp)
{
	VS_Output_Light outp = (VS_Output_Light)0;
	if (inp.tti >= 0)
	{
		outp.pos = lightTransPerspVP(mul(inp.pos, transarr[inp.tti]));
	}
	else
	{
		outp.pos = lightTransPerspVP(inp.pos);
	}
	outp.altPos = outp.pos;
	return outp;
}

VS_Output_Light VShade_Tex_LightPoint(VS_Input_Tex inp)
{
	VS_Output_Light outp = (VS_Output_Light)0;
	if (inp.tti >= 0)
	{
		outp.pos = lightTransPointVP(mul(inp.pos, transarr[inp.tti]));
	}
	else
	{
		outp.pos = lightTransPointVP(inp.pos);
	}
	outp.altPos = outp.pos;
	return outp;
}




// VShade_Sprite_Light
VS_Output_Light VShade_Sprite_LightOrtho(VS_Input_Tex inp)
{
	return (VS_Output_Light)0;
}


// VShade_Sprite_Lit
VS_Output_Light VShade_Sprite_LightPersp(VS_Input_Tex inp)
{
	return (VS_Output_Light)0;
}


// VShade_Sprite_Lit
VS_Output_Light VShade_Sprite_LightPoint(VS_Input_Tex inp)
{
	return (VS_Output_Light)0;
}



// VShade_Sprite
VS_Output_Tex VShade_Sprite(VS_Input_Tex inp)
{
	VS_Output_Tex outp = (VS_Output_Tex)0;
	float4 centre = (float4)0;
	if (inp.tti >= 0)
	{
		centre = spriteLoc[inp.tti];
	}
	inp.pos *= spriteDim;
	centre = mul(centre, viewMat);
	outp.pos = mul(centre + inp.pos, projMat);
	outp.altPos = outp.pos;
	outp.altPos.z = outp.altPos.z * outp.altPos.w * invFarDepth;
	outp.col = inp.col;
	outp.txc = inp.txc;

	return outp;
}

// VShade_Sprite_Lit
VS_Output_Tex VShade_Sprite_LitOrtho(VS_Input_Tex inp)
{
	VS_Output_Tex outp = (VS_Output_Tex)0;
	float4 centre = (float4)0;
	if (inp.tti >= 0)
	{
		centre = spriteLoc[inp.tti];
	}
	centre = mul(centre, viewProj);
	outp.pos = centre + inp.pos * spriteDim;
	outp.altPos = outp.pos;
	outp.altPos.z = outp.altPos.z * outp.altPos.w * invFarDepth;
	outp.col = inp.col;
	outp.txc = inp.txc;

	// light (not implemented, this is only here because I can't be bothered to remove it, I'd much rather type this, I make sense)
	outp.lit = lightLitnessOrtho(inp.pos, inp.nrm);
	outp.lmc = lightTransOrtho(inp.pos);

	return outp;
}

VS_Output_Tex VShade_Sprite_LitPersp(VS_Input_Tex inp)
{
	VS_Output_Tex outp = (VS_Output_Tex)0;
	float4 centre = (float4)0;
	if (inp.tti >= 0)
	{
		centre = spriteLoc[inp.tti];
	}
	centre = mul(centre, viewProj);
	outp.pos = centre + inp.pos * spriteDim;
	outp.altPos = outp.pos;
	outp.altPos.z = outp.altPos.z * outp.altPos.w * invFarDepth;
	outp.col = inp.col;
	outp.txc = inp.txc;

	// light
	outp.lit = lightLitnessPersp(inp.pos, inp.nrm);
	outp.lmc = lightTransPersp(inp.pos);

	return outp;
}

VS_Output_Tex VShade_Sprite_LitPoint(VS_Input_Tex inp)
{
	VS_Output_Tex outp = (VS_Output_Tex)0;
	float4 centre = (float4)0;
	if (inp.tti >= 0)
	{
		centre = spriteLoc[inp.tti];
	}
	centre = mul(centre, viewProj);
	outp.pos = centre + inp.pos * spriteDim;
	outp.altPos = outp.pos;
	outp.altPos.z = outp.altPos.z * outp.altPos.w * invFarDepth;
	outp.col = inp.col;
	outp.txc = inp.txc;

	// light
	outp.lit = lightLitnessPoint(inp.pos, inp.nrm);
	outp.lmc = lightTransPoint(inp.pos);

	return outp;
}



// VShade_Sprite_Fire
VS_Output_Tex VShade_Sprite_Fire(VS_Input_Tex inp)
{
	VS_Output_Tex outp = (VS_Output_Tex)0;
	float4 centre = (float4)0;
	if (inp.tti >= 0)
	{
		centre = spriteLoc[inp.tti];
	}
	float4 oth = spriteLoc[inp.tti + 1];
	inp.pos *= spriteDim;
	if (oth.x > oth.z)
	{
		inp.pos *= ((oth.y - oth.x) / (oth.y - oth.z));
	}
	centre = mul(centre, viewMat);
	outp.pos = mul(centre + inp.pos, projMat);
	outp.altPos = outp.pos;
	outp.altPos.z = outp.altPos.z * outp.altPos.w * invFarDepth;
	outp.col = inp.col;
	outp.col.w *= 1.0 - (oth.x / oth.w);
	outp.txc = inp.txc;

	return outp;
}

VS_Output_Tex VShade_Sprite_Fire_Radial_Offset(VS_Input_Tex inp)
{
	VS_Output_Tex outp = (VS_Output_Tex)0;
	float4 centre = (float4)0;
	if (inp.tti >= 0)
	{
		centre = spriteLoc[inp.tti];
	}
	float4 oth = spriteLoc[inp.tti + 1];
	inp.pos *= spriteDim;
	if (oth.x > oth.z)
	{
		inp.pos *= ((oth.y - oth.x) / (oth.y - oth.z));
	}
	centre = mul(centre, viewMat);
	outp.pos = mul(centre + inp.pos, projMat);
	outp.altPos = outp.pos;
	outp.col = inp.col;
	outp.txc = inp.txc;

	return outp;
}

// VShade_Sprite_Smoke
VS_Output_Tex VShade_Sprite_Smoke(VS_Input_Tex inp)
{
	VS_Output_Tex outp = (VS_Output_Tex)0;
	float4 centre = (float4)0;
	if (inp.tti >= 0)
	{
		centre = spriteLoc[inp.tti];
	}
	float4 oth = spriteLoc[inp.tti + 1];
	inp.pos *= spriteDim;
	outp.col = inp.col;
	if (oth.x > oth.w)
	{
		float smod = (oth.x - oth.w) / (1 - oth.w);
		inp.pos *= (1 + smod * oth.z);
		outp.col.w *= (1 - smod);
	}
	centre = mul(centre, viewMat);
	outp.pos = mul(centre + inp.pos, projMat);
	outp.altPos = outp.pos;
	outp.altPos.z = outp.altPos.z * outp.altPos.w * invFarDepth;
	outp.txc = inp.txc;

	return outp;
}





VS_Output_Decal VShade_Decal(VS_Input_Decal inp)
{
	VS_Output_Decal outp = (VS_Output_Decal)0;
	if (inp.tti >= 0)
	{
		inp.pos = mul(inp.pos, transarr[inp.tti]);
		inp.nrm = mul(inp.nrm, transarr[inp.tti]);
	}
	outp.pos = mul(inp.pos, viewProj);
	//outp.pos.z = outp.pos.z - 0.001;
	outp.altPos = outp.pos;
	outp.altPos.z = outp.altPos.z * outp.altPos.w * invFarDepth;
	outp.col = inp.col;
	outp.txc = inp.txc;

	return outp;
}

// VShade_Decal_Lit
VS_Output_Decal VShade_Decal_LitOrtho(VS_Input_Decal inp)
{
	VS_Output_Decal outp = (VS_Output_Decal)0;
	if (inp.tti >= 0)
	{
		inp.pos = mul(inp.pos, transarr[inp.tti]);
		inp.nrm = mul(inp.nrm, transarr[inp.tti]);
	}
	outp.pos = mul(inp.pos, viewProj);
	//outp.pos.z = outp.pos.z - 0.001;
	outp.altPos = outp.pos;
	outp.altPos.z = outp.altPos.z * outp.altPos.w * invFarDepth;
	outp.col = inp.col;
	outp.txc = inp.txc;

	// light
	outp.lit = lightLitnessOrtho(inp.pos, inp.nrm);
	outp.lmc = lightTransOrtho(inp.pos);

	return outp;
}

VS_Output_Decal VShade_Decal_LitPersp(VS_Input_Decal inp)
{
	VS_Output_Decal outp = (VS_Output_Decal)0;
	if (inp.tti >= 0)
	{
		inp.pos = mul(inp.pos, transarr[inp.tti]);
		inp.nrm = mul(inp.nrm, transarr[inp.tti]);
	}
	outp.pos = mul(inp.pos, viewProj);
	//outp.pos.z = outp.pos.z - 0.001;
	outp.altPos = outp.pos;
	outp.altPos.z = outp.altPos.z * outp.altPos.w * invFarDepth;
	outp.col = inp.col;
	outp.txc = inp.txc;

	// light
	outp.lit = lightLitnessPersp(inp.pos, inp.nrm);
	outp.lmc = lightTransPersp(inp.pos);

	return outp;
}

VS_Output_Decal VShade_Decal_LitPoint(VS_Input_Decal inp)
{
	VS_Output_Decal outp = (VS_Output_Decal)0;
	if (inp.tti >= 0)
	{
		inp.pos = mul(inp.pos, transarr[inp.tti]);
		inp.nrm = mul(inp.nrm, transarr[inp.tti]);
	}
	outp.pos = mul(inp.pos, viewProj);
	//outp.pos.z = outp.pos.z - 0.001;
	outp.altPos = outp.pos;
	outp.altPos.z = outp.altPos.z * outp.altPos.w * invFarDepth;
	outp.col = inp.col;
	outp.txc = inp.txc;

	// light
	outp.lit = lightLitnessPoint(inp.pos, inp.nrm);
	outp.lmc = lightTransPoint(inp.pos);

	return outp;
}





PS_Output PShade_Tex(VS_Output_Tex inp)
{
	//float num = (inp.altPos.z / inp.altPos.w);
	PS_Output outp = (PS_Output)0;
	//outp.dep = num;
	//num = 1.0 - num;
	outp.col = inp.col * tex2D(texSampler, inp.txc);

	float alphaPreserve = outp.col.w;
	//clip (alphaPreserve <= 0.0 ? -1 : 1); // freindly alpha clip

	outp.col = outp.col * colMod;

	outp.col = outp.col * (1.0 - lightCoof);

	outp.col.w = alphaPreserve;

	return outp;
}

/// TEST CODE
PS_Output PShade_Tex_Test(VS_Output_Tex inp)
{
	// altPos is NOT w depth!

	PS_Output outp = (PS_Output)0;
	float wNum = inp.altPos.w;

	inp.altPos = mul(inp.altPos, vpMat);
	float2 tcoords = float2(inp.altPos.x, inp.altPos.y);

	tcoords.x /= wNum;
	tcoords.y /= wNum;

	tcoords.x *= targTexData.z;
	tcoords.y *= targTexData.w;

	tcoords.x += targTexData.x;
	tcoords.y += targTexData.y;

	outp.col = tex2D(targTexSampler, tcoords);

	outp.col.x += 100;
	return outp;
}
/// END TEST CODE

// PShade_Tex_Lit
PS_Output PShade_Tex_LitOrtho(VS_Output_Tex inp)
{
	//clip(inp.lit);

	PS_Output outp = (PS_Output)0;

	outp.col = inp.col * tex2D(texSampler, inp.txc);

	float alphaPreserve = outp.col.w;
	//clip (alphaPreserve <= 0.0 ? -1 : 1); // freindly alpha clip

	float4 lightMod = calcLightModOrtho(inp.lmc);

	outp.col = outp.col * colMod;

	outp.col = outp.col * (lightMod * inp.lit + lightAmbient) * lightCoof;

	outp.col.w = alphaPreserve;

	return outp;
}

PS_Output PShade_Tex_LitPersp(VS_Output_Tex inp)
{
	//clip(inp.lit);

	PS_Output outp = (PS_Output)0;
	outp.col = inp.col * tex2D(texSampler, inp.txc);

	float alphaPreserve = outp.col.w;
	//clip (alphaPreserve <= 0.0 ? -1 : 1); // freindly alpha clip

	float4 lightMod = calcLightModPersp(inp.lmc);

	outp.col = outp.col * colMod;

	outp.col = outp.col * (lightMod * inp.lit + lightAmbient) * lightCoof;

	outp.col.w = alphaPreserve;

	return outp;
}

PS_Output PShade_Tex_LitPoint(VS_Output_Tex inp)
{
	//clip(inp.lit);

	PS_Output outp = (PS_Output)0;
	outp.col = inp.col * tex2D(texSampler, inp.txc);

	float alphaPreserve = outp.col.w;
	//clip (alphaPreserve <= 0.0 ? -1 : 1); // freindly alpha clip

	float4 lightMod = calcLightModPoint(inp.lmc);

	outp.col = outp.col * colMod;

	outp.col = outp.col * (lightMod * inp.lit + lightAmbient) * lightCoof;

	outp.col.w = alphaPreserve;

	return outp;
}




PS_Output PShade_Cloud(VS_Output_Tex inp)
{
//	float num = (inp.altPos.z / inp.altPos.w);
	PS_Output outp = (PS_Output)0;
	//outp.dep = num;
//	num = 1.0 - num;
	outp.col = inp.col * tex2D(texSampler, inp.txc);

	float alphaPreserve = outp.col.w;
	//clip (alphaPreserve <= 0.0 ? -1 : 1); // freindly alpha clip

	outp.col = outp.col * colMod;

	outp.col.w = alphaPreserve;

	outp.col.x = 0;

	return outp;
}

PS_Output PShade_Decal(VS_Output_Decal inp)
{
	/*clip(inp.txc.x);
	clip(1.0 - inp.txc.x);
	clip(inp.txc.y);
	clip(1.0 - inp.txc.y);
	clip(inp.txc.z);
	clip(1.0 - inp.txc.z);*/

	PS_Output outp = (PS_Output)0;
	
	float2 txc;
	txc.x = inp.txc.x;
	txc.y = inp.txc.y;
	outp.col = tex2D(texBorderSampler, txc) * colMod * inp.col;

	float alphaPreserve = outp.col.w;
	//clip (alphaPreserve <= 0.0 ? -1 : 1); // freindly alpha clip

	outp.col = outp.col * (1.0 - lightCoof);

	outp.col.w = alphaPreserve;

	return outp;
}

// PShade_Decal_Lit
PS_Output PShade_Decal_LitOrtho(VS_Output_Decal inp)
{
	//clip(inp.lit);

	/*clip(inp.txc.x);
	clip(1.0 - inp.txc.x);
	clip(inp.txc.y);
	clip(1.0 - inp.txc.y);
	clip(inp.txc.z);
	clip(1.0 - inp.txc.z);*/

	PS_Output outp = (PS_Output)0;
	
	float2 txc;
	txc.x = inp.txc.x;
	txc.y = inp.txc.y;
	outp.col = tex2D(texBorderSampler, txc) * colMod * inp.col;

	float alphaPreserve = outp.col.w;
	//clip (alphaPreserve <= 0.0 ? -1 : 1); // freindly alpha clip

	float4 lightMod = calcLightModOrtho(inp.lmc);

	outp.col = outp.col * (lightMod * inp.lit + lightAmbient) * lightCoof;

	outp.col.w = alphaPreserve;

	return outp;
}

PS_Output PShade_Decal_LitPersp(VS_Output_Decal inp)
{
	//clip(inp.lit);

	/*clip(inp.txc.x);
	clip(1.0 - inp.txc.x);
	clip(inp.txc.y);
	clip(1.0 - inp.txc.y);
	clip(inp.txc.z);
	clip(1.0 - inp.txc.z);*/

	PS_Output outp = (PS_Output)0;
	
	float2 txc;
	txc.x = inp.txc.x;
	txc.y = inp.txc.y;
	outp.col = tex2D(texBorderSampler, txc) * colMod * inp.col;

	float alphaPreserve = outp.col.w;
	//clip (alphaPreserve <= 0.0 ? -1 : 1); // freindly alpha clip

	float4 lightMod = calcLightModPersp(inp.lmc);

	outp.col = outp.col * (lightMod * inp.lit + lightAmbient) * lightCoof;

	outp.col.w = alphaPreserve;

	return outp;
}

PS_Output PShade_Decal_LitPoint(VS_Output_Decal inp)
{
	//clip(inp.lit);

	/*clip(inp.txc.x);
	clip(1.0 - inp.txc.x);
	clip(inp.txc.y);
	clip(1.0 - inp.txc.y);
	clip(inp.txc.z);
	clip(1.0 - inp.txc.z);*/

	PS_Output outp = (PS_Output)0;
	
	float2 txc;
	txc.x = inp.txc.x;
	txc.y = inp.txc.y;
	outp.col = tex2D(texBorderSampler, txc) * colMod * inp.col;

	float alphaPreserve = outp.col.w;
	//clip (alphaPreserve <= 0.0 ? -1 : 1); // freindly alpha clip

	float4 lightMod = calcLightModPoint(inp.lmc);

	outp.col = outp.col * (lightMod * inp.lit + lightAmbient) * lightCoof;

	outp.col.w = alphaPreserve;

	return outp;
}



// this is used for opaque stuff - if it contains alphaness, then it needs it's own Light PShade
// PShade_Light
PS_Output PShade_LightOrtho(VS_Output_Light inp)
{
	inp.altPos = lightUnTransOrtho(inp.altPos);

	float num = inp.altPos.z;
	PS_Output outp = (PS_Output)0;
	outp.col.x = num;

	return outp;
}

PS_Output PShade_LightPersp(VS_Output_Light inp)
{
	inp.altPos = lightUnTransPersp(inp.altPos);

	float num = inp.altPos.z;
	PS_Output outp = (PS_Output)0;
	outp.col.x = num;

	return outp;
}

PS_Output PShade_LightPoint(VS_Output_Light inp)
{
	inp.altPos = lightUnTransPoint(inp.altPos);

	float num = inp.altPos.z;
	PS_Output outp = (PS_Output)0;
	outp.col.x = num;

	return outp;
}




// clips translucent stuff (these don't work, need to ADAPTED for individual use, with a new set of VShades)
// PShade_Tex_Light
PS_Output PShade_Tex_LightOrtho(VS_Output_Tex inp)
{
	float4 testCol = inp.col * tex2D(texSampler, inp.txc);

	float alphaPreserve = testCol.w;
	clip (alphaPreserve <= 0.9 ? -1 : 1); // agressive alpha clip


	inp.altPos = lightUnTransOrtho(inp.altPos);

	float num = inp.altPos.z;

	PS_Output outp = (PS_Output)0;
	outp.col.x = num;

	return outp;
}

PS_Output PShade_Tex_LightPersp(VS_Output_Tex inp)
{
	float4 testCol = inp.col * tex2D(texSampler, inp.txc);

	float alphaPreserve = testCol.w;
	clip (alphaPreserve <= 0.9 ? -1 : 1); // agressive alpha clip


	inp.altPos = lightUnTransPersp(inp.altPos);

	float num = inp.altPos.z;
	PS_Output outp = (PS_Output)0;
	outp.col.x = num;

	return outp;
}

PS_Output PShade_Tex_LightPoint(VS_Output_Tex inp)
{
	float4 testCol = inp.col * tex2D(texSampler, inp.txc);

	float alphaPreserve = testCol.w;
	clip (alphaPreserve <= 0.9 ? -1 : 1); // agressive alpha clip


	inp.altPos = lightUnTransPoint(inp.altPos);

	float num = inp.altPos.z;
	PS_Output outp = (PS_Output)0;
	outp.col.x = num;

	return outp;
}



VS_Output_Terrain VShade_Terrain(VS_Input_Terrain inp)
{
	VS_Output_Terrain outp = (VS_Output_Terrain)0;
	outp.pos = mul(inp.pos, viewProj);
	outp.altPos = outp.pos;
	outp.altPos.z = outp.altPos.z * outp.altPos.w * invFarDepth;
	outp.orgY = inp.pos.y;
	outp.txc = inp.txc;
	outp.w = inp.w;

	return outp;
}

// VShade_Terrain_Lit
VS_Output_Terrain VShade_Terrain_LitOrtho(VS_Input_Terrain inp)
{
	VS_Output_Terrain outp = (VS_Output_Terrain)0;
	outp.pos = mul(inp.pos, viewProj);
	outp.altPos = outp.pos;
	outp.altPos.z = outp.altPos.z * outp.altPos.w * invFarDepth;
	outp.orgY = inp.pos.y;
	outp.txc = inp.txc;
	outp.w = inp.w;

	// light
	outp.lit = lightLitnessOrtho(inp.pos, inp.nrm);
	outp.lmc = lightTransOrtho(inp.pos);

	return outp;
}

VS_Output_Terrain VShade_Terrain_LitPersp(VS_Input_Terrain inp)
{
	VS_Output_Terrain outp = (VS_Output_Terrain)0;
	outp.pos = mul(inp.pos, viewProj);
	outp.altPos = outp.pos;
	outp.altPos.z = outp.altPos.z * outp.altPos.w * invFarDepth;
	outp.orgY = inp.pos.y;
	outp.txc = inp.txc;
	outp.w = inp.w;

	// light
	outp.lit = lightLitnessPersp(inp.pos, inp.nrm);
	outp.lmc = lightTransPersp(inp.pos);

	return outp;
}

VS_Output_Terrain VShade_Terrain_LitPoint(VS_Input_Terrain inp)
{
	VS_Output_Terrain outp = (VS_Output_Terrain)0;
	outp.pos = mul(inp.pos, viewProj);
	outp.altPos = outp.pos;
	outp.altPos.z = outp.altPos.z * outp.altPos.w * invFarDepth;
	outp.orgY = inp.pos.y;
	outp.txc = inp.txc;
	outp.w = inp.w;

	// light
	outp.lit = lightLitnessPoint(inp.pos, inp.nrm);
	outp.lmc = lightTransPoint(inp.pos);

	return outp;
}

// VShade_Terrain_DynamicDecal
VS_Output_Dyn VShade_Terrain_DynOrtho(VS_Input_Terrain inp)
{
	VS_Output_Dyn outp = (VS_Output_Dyn)0;
	outp.pos = mul(inp.pos, viewProj);
	outp.altPos = outp.pos;
	outp.altPos.z = outp.altPos.z * outp.altPos.w * invFarDepth;
	outp.lmc = lightTransOrtho(inp.pos);

	return outp;
}

VS_Output_Dyn VShade_Terrain_DynPersp(VS_Input_Terrain inp)
{
	VS_Output_Dyn outp = (VS_Output_Dyn)0;
	outp.pos = mul(inp.pos, viewProj);
	outp.altPos = outp.pos;
	outp.altPos.z = outp.altPos.z * outp.altPos.w * invFarDepth;
	outp.lmc = lightTransPersp(inp.pos);

	return outp;
}


// VShade_Terrain_Light
VS_Output_Light VShade_Terrain_LightOrtho(VS_Input_Terrain inp)
{
	VS_Output_Light outp = (VS_Output_Light)0;
	outp.pos = lightTransOrthoVP(inp.pos);
	outp.altPos = outp.pos;

	return outp;
}

VS_Output_Light VShade_Terrain_LightPersp(VS_Input_Terrain inp)
{
	VS_Output_Light outp = (VS_Output_Light)0;
	outp.pos = lightTransPerspVP(inp.pos);
	outp.altPos = outp.pos;

	return outp;
}

VS_Output_Light VShade_Terrain_LightPoint(VS_Input_Terrain inp)
{
	VS_Output_Light outp = (VS_Output_Light)0;
	outp.pos = lightTransPointVP(inp.pos);
	outp.altPos = outp.pos;

	return outp;
}





// based off VShade_Tex_Decal
VS_Output_Decal VShade_Terrain_Decal(VS_Input_Decal inp)
{
	VS_Output_Decal outp = (VS_Output_Decal)0;
	outp.pos = mul(inp.pos, viewProj);
	//outp.pos.z = outp.pos.z - 0.001;
	outp.altPos = outp.pos;
	outp.altPos.z = outp.altPos.z * outp.altPos.w * invFarDepth;
	outp.col = inp.col;
	outp.txc = inp.txc;

	return outp;
}

// VShade_Terrain_Decal_Lit
VS_Output_Decal VShade_Terrain_Decal_LitOrtho(VS_Input_Decal inp)
{
	VS_Output_Decal outp = (VS_Output_Decal)0;
	outp.pos = mul(inp.pos, viewProj);
	//outp.pos.z = outp.pos.z - 0.001;
	outp.altPos = outp.pos;
	outp.altPos.z = outp.altPos.z * outp.altPos.w * invFarDepth;
	outp.col = inp.col;
	outp.txc = inp.txc;

	// light
	outp.lit = lightLitnessOrtho(inp.pos, inp.nrm);
	outp.lmc = lightTransOrtho(inp.pos);

	return outp;
}

VS_Output_Decal VShade_Terrain_Decal_LitPersp(VS_Input_Decal inp)
{
	VS_Output_Decal outp = (VS_Output_Decal)0;
	outp.pos = mul(inp.pos, viewProj);
	//outp.pos.z = outp.pos.z - 0.001;
	outp.altPos = outp.pos;
	outp.altPos.z = outp.altPos.z * outp.altPos.w * invFarDepth;
	outp.col = inp.col;
	outp.txc = inp.txc;

	// light
	outp.lit = lightLitnessPersp(inp.pos, inp.nrm);
	outp.lmc = lightTransPersp(inp.pos);

	return outp;
}

VS_Output_Decal VShade_Terrain_Decal_LitPoint(VS_Input_Decal inp)
{
	VS_Output_Decal outp = (VS_Output_Decal)0;
	outp.pos = mul(inp.pos, viewProj);
	//outp.pos.z = outp.pos.z - 0.001;
	outp.altPos = outp.pos;
	outp.altPos.z = outp.altPos.z * outp.altPos.w * invFarDepth;
	outp.col = inp.col;
	outp.txc = inp.txc;

	// light
	outp.lit = lightLitnessPoint(inp.pos, inp.nrm);
	outp.lmc = lightTransPoint(inp.pos);

	return outp;
}






PS_Output PShade_Terrain(VS_Output_Terrain inp)
{
	//float num = (inp.altPos.z / inp.altPos.w);
	PS_Output outp = (PS_Output)0;
	//outp.dep = num;
	//num = 1.0 - num;
	outp.col = (inp.w.x * tex2D(tex0Sampler, inp.txc) + inp.w.y * tex2D(tex1Sampler, inp.txc) + inp.w.z * tex2D(tex2Sampler, inp.txc) + inp.w.w * tex2D(tex3Sampler, inp.txc));
	
	float alphaPreserve = outp.col.w;

	outp.col = outp.col * (1.0 - lightCoof);

	outp.col.w = alphaPreserve;

	return outp;
}

// PShade_Terrain_Lit
PS_Output PShade_Terrain_LitOrtho(VS_Output_Terrain inp)
{
	//clip(inp.lit);

	PS_Output outp = (PS_Output)0;
	outp.col = (inp.w.x * tex2D(tex0Sampler, inp.txc) + inp.w.y * tex2D(tex1Sampler, inp.txc) + inp.w.z * tex2D(tex2Sampler, inp.txc) + inp.w.w * tex2D(tex3Sampler, inp.txc));
	
	float4 lightMod = calcLightModOrtho(inp.lmc);

	outp.col = outp.col * (lightMod * inp.lit + lightAmbient) * lightCoof;

	return outp;
}

PS_Output PShade_Terrain_LitPersp(VS_Output_Terrain inp)
{
	//clip(inp.lit);

	PS_Output outp = (PS_Output)0;
	outp.col = (inp.w.x * tex2D(tex0Sampler, inp.txc) + inp.w.y * tex2D(tex1Sampler, inp.txc) + inp.w.z * tex2D(tex2Sampler, inp.txc) + inp.w.w * tex2D(tex3Sampler, inp.txc));

	float4 lightMod = calcLightModPersp(inp.lmc);

	outp.col = outp.col * (lightMod * inp.lit + lightAmbient) * lightCoof;

	return outp;
}

PS_Output PShade_Terrain_LitPoint(VS_Output_Terrain inp)
{
	//clip(inp.lit);

	PS_Output outp = (PS_Output)0;
	outp.col = (inp.w.x * tex2D(tex0Sampler, inp.txc) + inp.w.y * tex2D(tex1Sampler, inp.txc) + inp.w.z * tex2D(tex2Sampler, inp.txc) + inp.w.w * tex2D(tex3Sampler, inp.txc));

	float4 lightMod = calcLightModPoint(inp.lmc);

	outp.col = outp.col * (lightMod * inp.lit + lightAmbient) * lightCoof;

	return outp;
}




// PShade_Terrain_Dyn (amicDecal)
//  - note that the DynamicDecal and Dyn shaders and techniques depend on the lit shaders
//    and are very, very similiar, and use the same shader variables/handles
PS_Output PShade_Terrain_DynOrtho(VS_Output_Dyn inp)
{
	PS_Output outp = (PS_Output)0;
	
	float4 lightMod = calcDynModOrtho(inp.lmc);

	outp.col = lightMod;

	return outp;
}

PS_Output PShade_Terrain_DynPersp(VS_Output_Dyn inp)
{
	PS_Output outp = (PS_Output)0;

	float4 lightMod = calcDynModPersp(inp.lmc);

	outp.col = lightMod;

	return outp;
}





// side/over shaders

VS_Output_Over VShade_Over(VS_Input_Over inp)
{
	VS_Output_Over outp = (VS_Output_Over)0;
	outp.pos = mul(inp.pos, viewProj);
	outp.altPos = outp.pos;
	outp.txc = inp.txc;
	return outp;
}

PS_Output PShade_Over(VS_Output_Over inp)
{
	PS_Output outp = (PS_Output)0;

	outp.col = tex2D(texSampler, inp.txc);

	return outp;
}

VS_Output_Over VShade_Over_Final(VS_Input_Over inp)
{
	VS_Output_Over outp = (VS_Output_Over)0;
	outp.pos = mul(inp.pos, viewProj);
	outp.altPos = outp.pos;
	outp.txc = inp.txc;
	return outp;
}

PS_Output PShade_Over_Final(VS_Output_Over inp)
{
	PS_Output outp = (PS_Output)0;

	outp.col = tex2D(texSampler, inp.txc);

	return outp;
}

PS_Output PShade_Over_Linear(VS_Output_Over inp)
{
	PS_Output outp = (PS_Output)0;

	outp.col = tex2D(texLinearSampler, inp.txc);

	return outp;
}

PS_Output PShade_Over_Greys(VS_Output_Over inp)
{
	PS_Output outp = (PS_Output)0;

	outp.col = tex2D(texSampler, inp.txc);
	
	//float roottwo = 1.4142135623730950488016887242097;
	float two = 2.00000000000000000000000000000000;
	float nx = (inp.txc.x - 0.5) * two;
	float ny = (inp.txc.y - 0.5) * two;
	float num = sqrt(nx * nx + ny * ny);

	num *= num;

	num *= 1.2;
	num -= 0.2;
	if (num < 0)
	{
		num = 0;
	}
	if (num > 1)
	{
		num = 1;
	}

	float grey = (outp.col.x + outp.col.y + outp.col.z) / 3;

	
	outp.col.x = outp.col.x * (1.0 - num) + grey * num;
	outp.col.y = outp.col.y * (1.0 - num) + grey * num;
	outp.col.z = outp.col.z * (1.0 - num) + grey * num;

	return outp;
}

PS_Output PShade_Over_Final_Fuzzy(VS_Output_Over inp)
{
	PS_Output outp = (PS_Output)0;

	outp.col = tex2D(texSampler, inp.txc) * 0.6;

	inp.txc.x -= texData.z;
	inp.txc.y -= texData.w;
	outp.col += tex2D(texSampler, inp.txc) * 0.1;

	inp.txc.x += texData.z * 2;
	outp.col += tex2D(texSampler, inp.txc) * 0.1;

	inp.txc.y += texData.w * 2;
	outp.col += tex2D(texSampler, inp.txc) * 0.1;

	inp.txc.x -= texData.z * 2;
	outp.col += tex2D(texSampler, inp.txc) * 0.1;

	return outp;
}

PS_Output PShade_Over_Fuzz(VS_Output_Over inp)
{ // uses tex0
	PS_Output outp = (PS_Output)0;

	float4 off = tex2D(tex0Sampler, inp.txc);

	float mod = 0.005 - 0.01 * ((ticker * 10) % 1);
	inp.txc.x += off.x;
	inp.txc.y += off.y;

	outp.col = tex2D(texSampler, inp.txc);

	return outp;
}


PS_Output PShade_Over_Offset(VS_Output_Over inp)
{
	PS_Output outp = (PS_Output)0;

	float4 offCol = tex2D(tex1Sampler, inp.txc);
	float2 offset;

	offset.x = offCol.x;
	offset.y = offCol.y;

	offset.x -= 0.5 * offCol.z;
	offset.y -= 0.5 * offCol.z;

	//outp.col = tex2D(tex0Sampler, inp.txc);

	offset.x *= 0.2;
	offset.y *= 0.2;

	//offset *= ticker;

	inp.txc += offset;

	outp.col = tex2D(tex0Sampler, inp.txc);

/*
	outp.col.x = inp.txc.x;
	outp.col.y = inp.txc.y;
	outp.col.z = 0;
	outp.col.w = 1;
*/

	return outp;
}

PS_Output PShade_Tex_Radials(VS_Output_Tex inp)
{
	PS_Output outp = (PS_Output)0;

	float4 offCol = tex2D(texSampler, inp.txc);

	outp.col.x = (inp.txc.x - 0.5);
	outp.col.y = -(inp.txc.y - 0.5);

	outp.col *= (offCol.x - offCol.y) * colMod.x;

	outp.col.x += 0.5;
	outp.col.y += 0.5;

	outp.col.z = 1;
	outp.col.w = 1;

	return outp;
}

PS_Output PShade_Tex_Radial_Offset(VS_Output_Tex inp)
{
	PS_Output outp = (PS_Output)0;

	float4 offCol = tex2D(texSampler, inp.txc);

	float2 offTxc = float2(offCol.x, offCol.y);
	offTxc *= colMod.x;


	// altPos is NOT w depth!
	float wNum = inp.altPos.w;

	inp.altPos = mul(inp.altPos, vpMat);
	float2 tcoords = float2(inp.altPos.x, inp.altPos.y);

	tcoords.x /= wNum;
	tcoords.y /= wNum;

	tcoords.x *= targTexData.z;
	tcoords.y *= targTexData.w;

	tcoords.x += targTexData.x;
	tcoords.y += targTexData.y;

	tcoords.x += offTxc.x;
	tcoords.y += offTxc.y;

	outp.col = tex2D(targTexSampler, tcoords);


	return outp;
}





// all lit techniques must have:
// P0 - no lighting
// P1 - Ortho Lit
// P2 - Persp Lit
// P3 - Point Lit


// all light techniques must have:
// P0 - Ortho Light
// P1 - Persp Light
// P2 - Point Light



technique dull
{
	pass unlit
	{
		VertexShader = compile vs_2_0 VShade_Dull();
		PixelShader = compile ps_2_0 PShade_Dull();
	}
}

technique simpleUi
{
	pass unlit
	{
		VertexShader = compile vs_2_0 VShade_Ui();
		PixelShader = compile ps_2_0 PShade_Ui();
	}
}

technique simple
{
	pass unlit
	{
		VertexShader = compile vs_2_0 VShade();
		PixelShader = compile ps_2_0 PShade();
	}
}

technique simpleTex
{
	pass unlit
	{
		VertexShader = compile vs_2_0 VShade_Tex();
		PixelShader = compile ps_2_0 PShade_Tex();
	}

	pass litortho
	{ // LitOrtho
		VertexShader = compile vs_2_0 VShade_Tex_LitOrtho();
		PixelShader = compile ps_2_0 PShade_Tex_LitOrtho();
	}

	pass litpersp
	{ // LitPersp
		VertexShader = compile vs_2_0 VShade_Tex_LitPersp();
		PixelShader = compile ps_2_0 PShade_Tex_LitPersp();
	}

	pass litpoint
	{ // LitPoint
		VertexShader = compile vs_2_0 VShade_Tex_LitPoint();
		PixelShader = compile ps_2_0 PShade_Tex_LitPoint();
	}
}

/// TEST CODE
technique simpleTex_Test
{
	pass unlit
	{
		VertexShader = compile vs_2_0 VShade_Tex_Test();
		PixelShader = compile ps_2_0 PShade_Tex_Test();
	}

	pass litortho
	{ // LitOrtho
		VertexShader = compile vs_2_0 VShade_Tex_LitOrtho();
		PixelShader = compile ps_2_0 PShade_Tex_LitOrtho();
	}

	pass litpersp
	{ // LitPersp
		VertexShader = compile vs_2_0 VShade_Tex_LitPersp();
		PixelShader = compile ps_2_0 PShade_Tex_LitPersp();
	}

	pass litpoint
	{ // LitPoint
		VertexShader = compile vs_2_0 VShade_Tex_LitPoint();
		PixelShader = compile ps_2_0 PShade_Tex_LitPoint();
	}
}
/// END TEST CODE

technique simpleTex_light
{
	pass lightortho
	{ // LightOrtho
		VertexShader = compile vs_2_0 VShade_Tex_LightOrtho();
		PixelShader = compile ps_2_0 PShade_LightOrtho(); // no trans
	}

	pass lightpersp
	{ // LightPersp
		VertexShader = compile vs_2_0 VShade_Tex_LightPersp();
		PixelShader = compile ps_2_0 PShade_LightPersp();
	}

	pass lightpoint
	{ // LightPoint - this shoudln't do anthing.... it should never be called!
		VertexShader = compile vs_2_0 VShade_Tex_LightPoint();
		PixelShader = compile ps_2_0 PShade_LightPoint();
	}
}

technique sprite
{
	pass unlit
	{
		VertexShader = compile vs_2_0 VShade_Sprite();
		PixelShader = compile ps_2_0 PShade_Tex();
	}

	pass litortho
	{ // LitOrtho
		VertexShader = compile vs_2_0 VShade_Sprite_LitOrtho();
		PixelShader = compile ps_2_0 PShade_Tex_LitOrtho();
	}

	pass litpersp
	{ // LitPersp
		VertexShader = compile vs_2_0 VShade_Sprite_LitPersp();
		PixelShader = compile ps_2_0 PShade_Tex_LitPersp();
	}

	pass litpoint
	{ // LitPoint
		VertexShader = compile vs_2_0 VShade_Sprite_LitPoint();
		PixelShader = compile ps_2_0 PShade_Tex_LitPoint();
	}
}

technique sprite_fire
{
	pass unlit
	{
		VertexShader = compile vs_2_0 VShade_Sprite_Fire();
		PixelShader = compile ps_2_0 PShade_Tex();
	}
}

technique sprite_smoke
{
	pass unlit
	{
		VertexShader = compile vs_2_0 VShade_Sprite_Smoke();
		PixelShader = compile ps_2_0 PShade_Tex();
	}
}

technique sprite_fire_radial_offset
{
	pass unlit
	{
		VertexShader = compile vs_2_0 VShade_Sprite_Fire_Radial_Offset();
		PixelShader = compile ps_2_0 PShade_Tex_Radial_Offset();
	}
}

technique sprite_light
{
	pass lightortho
	{ // LightOrtho
		VertexShader = compile vs_2_0 VShade_Sprite_LightOrtho();
		PixelShader = compile ps_2_0 PShade_LightOrtho(); // no trans
	}

	pass lightpersp
	{ // LightPersp
		VertexShader = compile vs_2_0 VShade_Sprite_LightPersp();
		PixelShader = compile ps_2_0 PShade_LightPersp();
	}

	pass lightpoint
	{ // LightPoint
		VertexShader = compile vs_2_0 VShade_Sprite_LightPoint();
		PixelShader = compile ps_2_0 PShade_LightPoint();
	}
}

// custom for clouds
technique cloud
{
	pass unlit
	{
		VertexShader = compile vs_2_0 VShade_Tex();
		PixelShader = compile ps_2_0 PShade_Cloud();
	}
}

technique simple_decal
{
	pass unlit
	{
		VertexShader = compile vs_2_0 VShade_Decal();
		PixelShader = compile ps_2_0 PShade_Decal();
	}

	pass litortho
	{ // LitOrtho
		VertexShader = compile vs_2_0 VShade_Decal_LitOrtho();
		PixelShader = compile ps_2_0 PShade_Decal_LitOrtho();
	}

	pass litpersp
	{ // LitPersp
		VertexShader = compile vs_2_0 VShade_Decal_LitPersp();
		PixelShader = compile ps_2_0 PShade_Decal_LitPersp();
	}

	pass litpoint
	{ // LitPoint
		VertexShader = compile vs_2_0 VShade_Decal_LitPoint();
		PixelShader = compile ps_2_0 PShade_Decal_LitPoint();
	}
}

technique terrain
{
	pass unlit
	{
		VertexShader = compile vs_2_0 VShade_Terrain();
		PixelShader = compile ps_2_0 PShade_Terrain();
	}

	pass litortho
	{ // LitOrtho
		VertexShader = compile vs_2_0 VShade_Terrain_LitOrtho();
		PixelShader = compile ps_2_0 PShade_Terrain_LitOrtho();
	}

	pass litpersp
	{ // LitPersp
		VertexShader = compile vs_2_0 VShade_Terrain_LitPersp();
		PixelShader = compile ps_2_0 PShade_Terrain_LitPersp();
	}

	pass litpoint
	{ // LitPoint
		VertexShader = compile vs_2_0 VShade_Terrain_LitPoint();
		PixelShader = compile ps_2_0 PShade_Terrain_LitPoint();
	}
}

technique terrain_decal
{
	pass unlit
	{
		VertexShader = compile vs_2_0 VShade_Terrain_Decal();
		PixelShader = compile ps_2_0 PShade_Decal();
	}

	pass litortho
	{ // LitOrtho
		VertexShader = compile vs_2_0 VShade_Terrain_Decal_LitOrtho();
		PixelShader = compile ps_2_0 PShade_Decal_LitOrtho();
	}

	pass litpersp
	{ // LitPersp
		VertexShader = compile vs_2_0 VShade_Terrain_Decal_LitPersp();
		PixelShader = compile ps_2_0 PShade_Decal_LitPersp();
	}

	pass litpoint
	{ // LitPoint
		VertexShader = compile vs_2_0 VShade_Terrain_Decal_LitPoint();
		PixelShader = compile ps_2_0 PShade_Decal_LitPoint();
	}
}

technique terrain_dynamicdecal
{
	pass dynortho
	{ // DynOrtho
		VertexShader = compile vs_2_0 VShade_Terrain_DynOrtho();
		PixelShader = compile ps_2_0 PShade_Terrain_DynOrtho();
	}

	pass dynpersp
	{ // DynPersp
		VertexShader = compile vs_2_0 VShade_Terrain_DynPersp();
		PixelShader = compile ps_2_0 PShade_Terrain_DynPersp();
	}
}

technique terrain_light
{
	pass lightortho
	{ // LightOrtho
		VertexShader = compile vs_2_0 VShade_Terrain_LightOrtho();
		PixelShader = compile ps_2_0 PShade_LightOrtho(); // no transparencies, keep it simples
	}

	pass lightpersp
	{ // LightPersp
		VertexShader = compile vs_2_0 VShade_Terrain_LightPersp();
		PixelShader = compile ps_2_0 PShade_LightPersp();
	}
	
	pass lightpoint
	{ // LightPoint
		VertexShader = compile vs_2_0 VShade_Terrain_LightPoint();
		PixelShader = compile ps_2_0 PShade_LightPoint();
	}
}

technique radialTex
{
	pass unlit
	{
		VertexShader = compile vs_2_0 VShade_Tex();
		PixelShader = compile ps_2_0 PShade_Tex_Radials();
	}
}

technique over
{
	pass over
	{
		VertexShader = compile vs_2_0 VShade_Over();
		PixelShader = compile ps_2_0 PShade_Over();
	}
}

technique over_final
{
	pass over
	{
		VertexShader = compile vs_2_0 VShade_Over_Final();
		PixelShader = compile ps_2_0 PShade_Over_Final();
		//VertexShader = compile vs_2_0 VShade_Over_Final();
		//PixelShader = compile ps_2_0 PShade_Over_Final_Fuzzy();
	}
}

technique over_linear
{
	pass over
	{
		VertexShader = compile vs_2_0 VShade_Over_Final();
		PixelShader = compile ps_2_0 PShade_Over_Linear();
	}
}

technique over_greys
{
	pass over
	{
		VertexShader = compile vs_2_0 VShade_Over_Final();
		PixelShader = compile ps_2_0 PShade_Over_Greys();
	}
}

technique over_fuzzy
{
	pass over
	{
		VertexShader = compile vs_2_0 VShade_Over_Final();
		PixelShader = compile ps_2_0 PShade_Over_Final_Fuzzy();
	}
}

technique over_offset
{
	pass over
	{
		VertexShader = compile vs_2_0 VShade_Over_Final();
		PixelShader = compile ps_2_0 PShade_Over_Offset();
	}
}