varying vec2 vTexCoord;

uniform float uTime;
uniform float uStrength;
uniform float uScale;
uniform vec2 uResolution;

// Simple hash function for noise
float hash(vec2 p)
{
	return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

// 2D noise function
float noise(vec2 p)
{
	vec2 i = floor(p);
	vec2 f = fract(p);
	f = f * f * (3.0 - 2.0 * f);
	
	float a = hash(i);
	float b = hash(i + vec2(1.0, 0.0));
	float c = hash(i + vec2(0.0, 1.0));
	float d = hash(i + vec2(1.0, 1.0));
	
	return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

// Layered noise for more interesting distortion
float fbm(vec2 p)
{
	float value = 0.0;
	float amplitude = 0.5;
	float frequency = 1.0;
	
	for (int i = 0; i < 4; i++)
	{
		value += amplitude * noise(p * frequency);
		frequency *= 2.0;
		amplitude *= 0.5;
	}
	
	return value;
}

void main()
{
	vec2 uv = vTexCoord;
	
	// Create animated heat distortion pattern
	vec2 distortCoord = vTexCoord * uScale + vec2(0.0, uTime * 0.1);
	
	// Generate distortion using noise
	float noiseX = fbm(distortCoord + vec2(uTime * 0.3, 0.0));
	float noiseY = fbm(distortCoord + vec2(0.0, uTime * 0.2));
	
	// Create wave-like distortion
	vec2 distortion = vec2(noiseX, noiseY) * 2.0 - 1.0;
	distortion *= uStrength;
	
	// Apply distortion to UV coordinates
	vec2 distortedUV = uv + distortion;
	
	// Sample the texture with distorted coordinates
	vec4 color = texture2D(gm_BaseTexture, distortedUV);
	
	// Reverse Bound
	if (distortedUV.x < 0.0 || distortedUV.x > 1.0)
		color = texture2D(gm_BaseTexture, vec2(-distortedUV.x, distortedUV.y));
		
	if (distortedUV.y < 0.0 || distortedUV.y > 1.0)
		color = texture2D(gm_BaseTexture, vec2(distortedUV.x, -distortedUV.y));
	
	gl_FragColor = color;
}