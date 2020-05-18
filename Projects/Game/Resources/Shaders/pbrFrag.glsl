#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUv;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in vec3 camPos;

layout(location = 0) out vec4 outColor;

layout(set=2, binding=0) uniform sampler2D baseColorTexture;
layout(set=2, binding=1) uniform sampler2D metallicRoughnessTexture;
layout(set=2, binding=2) uniform sampler2D normalTexture;
layout(set=2, binding=3) uniform sampler2D occlusionTexture;
layout(set=2, binding=4) uniform sampler2D emissiveTexture;

layout(set=4, binding=0) uniform samplerCube environmentMap;
layout(set=4, binding=1) uniform samplerCube irradianceMap;
layout(set=4, binding=2) uniform samplerCube prefilteredEnvMap;
layout(set=4, binding=3) uniform sampler2D brdfLutTexture;

layout(push_constant) uniform PushConstantsFrag
{
    vec4 baseColorFactor;
	vec4 emissiveFactor;
	float metallicFactor;
	float roughnessFactor;
	int baseColorTextureCoord;
	int metallicRoughnessTextureCoord;
	int normalTextureCoord;
	int occlusionTextureCoord;
	int emissiveTextureCoord;
    int padding;
};

const float PI = 3.14159265359;
const float MIN_ROUGHNESS = 0.04;
const float ALPHA_CUTOFF = 0.1;

vec4 SRGBtoLINEAR(vec4 color);
vec3 getNormal();
vec4 getSurfaceColor(vec2 uv);
vec2 getMetallicAndRoughness(vec2 uv);

/*
    Calculates the normal distribution function. Will give a value close to 1.0 if the microfacets on surface with normal N align to the halfway vector H.
    With respect to the surface roughness.
*/
float DistibutionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N,H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

/*
    k is the remapping of the roughness a.
    For direct lightning: k = (a+2)^2 / 8
    For IBL lightning: k = a^2 / 2
*/
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}

/*
    Calculates the geometry function. Gives an approximation of the relative surface area where its micro surface-details overshadow each other.
    Returns a 1.0 if no shadow, and 0.0 if shadow or something in between.
    N is the normal of the surface, V is the view direction and L is the light direction.
*/
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

/*
    Calculates the fresnel factor, which describes the ratio of light that gets reflected over the light that gets refracted.
    The function returns 0.0 if no relection, 1.0 if it is refleted.
    cosTheta is calculated like this: dot(H, V)
    F0 is the base reflectivity of the surface.
*/
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

float calculateAttenuation(vec3 p, vec3 lp)
{
    // attenuation = 1 / distance^2
    vec3 pToLp = lp - p;  
    return 1./dot(pToLp, pToLp);
}

vec3 getAmbientFromEnvironment(vec3 N, vec3 V, vec3 F0, float roughness, vec3 albedo)
{
    vec3 Ks = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    vec3 Kd = 1.0 - Ks;
    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse = irradiance * albedo;
    vec3 ambient = (Kd * diffuse);
    return ambient;
}

void main() {
    // Get surface color.
    vec4 surfaceColor = getSurfaceColor(fragUv);
	if(surfaceColor.a < ALPHA_CUTOFF)
		discard;

    // Get metallic and grouhness factors.
    vec2 metallicAndRoughness = getMetallicAndRoughness(fragUv);
    float metallic = metallicAndRoughness.x;
    float roughness  = metallicAndRoughness.y;

    vec3 N = getNormal();
    vec3 V = normalize(camPos - fragPos);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, surfaceColor.rgb, metallic);

    vec3 R = reflect(-V, N);

    // Also known as Cdiff
    vec3 diffuseAlbedo = surfaceColor.rgb;//mix(surfaceColor.rgb * (1.0 - 0.04), vec3(0.0), metallic);

    // This is the radiance for a point light. 
    // Example on other light sources: A directional light does not have attenuation and has a constant value of Wi.
/*  vec3 lightPos = vec3(3.0, 4.0, 2.0);
    vec3 lightColor = vec3(23.47, 21.21, 20.79);
    vec3 wi = normalize(lightPos - fragPos);
    float cosTheta = max(dot(N, wi), 0.0);
    float attenuation = calculateAttenuation(fragPos, lightPos);
    vec3 radiance = lightColor * attenuation * cosTheta;
*/
    vec3 lightPositions[1] = vec3[1](vec3(30.0, 20.0, 2.0));
    vec3 lightColors[1] = vec3[1](vec3(0.0));
    int numLights = 1;

    vec3 Lo = vec3(0.0);
    for(int i = 0; i < numLights; ++i)
    {
        vec3 L = normalize(lightPositions[i]-fragPos);
        vec3 H = normalize(V+L);
        // Use a directional light for now.
        float attenuation = 1.0;//calculateAttenuation(fragPos, lightPositions[i]);
        vec3 radiance = lightColors[i] * attenuation;

        // BRDF calculcation.
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        float NDF = DistibutionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        // Cook-Torrance BRDF
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
        vec3 specular = numerator / max(denominator, 0.001);

        vec3 Ks = F;
        vec3 Kd = vec3(1.0) - Ks;
        Kd *= 1.0 - metallic;

        float NdotL = max(dot(N, L), 0.0);
        Lo += (Kd * diffuseAlbedo / PI + specular) * radiance * NdotL;
    }

    // IBL calculation.
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    vec3 Ks = F;
    vec3 Kd = 1.0 - Ks;
    Kd *= 1.0 - metallic;

    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse = irradiance * diffuseAlbedo;

    float envMapDim = float(textureSize(prefilteredEnvMap, 0).s);
    float MAX_REFLECTION_LOD = log2(envMapDim);
    vec3 prelilteredColor = textureLod(prefilteredEnvMap, R, roughness * MAX_REFLECTION_LOD).rgb;
    vec2 envBRDF = texture(brdfLutTexture, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prelilteredColor * (F * envBRDF.x + envBRDF.y);
    vec3 ambient = (Kd * diffuse + specular);

    // Apply ambient occlusion term.
    const float u_OcclusionStrength = 1.0f;
	if (occlusionTextureCoord > -1) {
		float ao = texture(occlusionTexture, fragUv).r;
		ambient = mix(ambient, ambient * ao, u_OcclusionStrength);
	}
    vec3 color = ambient + Lo;

    // Apply emissive light.
    if (emissiveTextureCoord > -1) {
		vec3 emissive = SRGBtoLINEAR(texture(emissiveTexture, fragUv)).rgb * emissiveFactor.rgb;
		color += emissive;
	}

    // Tone map the HDR color using Reinhard operator.
    color = color / (color + vec3(1.0));
    // Gamma correction.
    color = pow(color, vec3(1.0/2.2)); 

    outColor = vec4(color, surfaceColor.a);
}

vec4 SRGBtoLINEAR(vec4 color)
{
    return vec4(pow(color.rgb, vec3(2.2)), color.a);
}

vec3 getNormal()
{
	// Perturb normal, see http://www.thetenthplanet.de/archives/1180
	vec3 tangentNormal = texture(normalTexture, fragUv).xyz * 2.0 - 1.0;

	vec3 q1 = dFdx(fragPos);
	vec3 q2 = dFdy(fragPos);
	vec2 st1 = dFdx(fragUv);
	vec2 st2 = dFdy(fragUv);

	vec3 N = normalize(fragNormal);
	vec3 T = normalize(q1 * st2.t - q2 * st1.t);
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

    return (normalTextureCoord > -1) ? normalize(TBN * tangentNormal) : normalize(fragNormal);
}

vec4 getSurfaceColor(vec2 uv)
{
    vec4 surfaceColor = vec4(0.0);  
	if(baseColorTextureCoord > -1)
		surfaceColor = SRGBtoLINEAR(texture(baseColorTexture, uv))*baseColorFactor; 
	else
		surfaceColor = baseColorFactor;
    return surfaceColor;
}

vec2 getMetallicAndRoughness(vec2 uv)
{
    // This is how the metallic and roughness factors should be calculated if gltf are used. If some other format is used, this might not work.

    float metallic = metallicFactor;
    float roughness  = roughnessFactor;
    if(metallicRoughnessTextureCoord > -1)
	{
        vec2 roughnessMetallic = texture(metallicRoughnessTexture, uv).gb;
		roughness  = roughnessMetallic.x*roughness;
		metallic = roughnessMetallic.y*metallic;
	}
	else
	{
		roughness  = clamp(roughness, MIN_ROUGHNESS, 1.0);
		metallic = clamp(metallic, 0.0, 1.0);
	}

    return vec2(metallic, roughness);
}