varying vec2 vTexCoord;

uniform vec2 uScreenSize;
uniform float uTime;
uniform float uDistortion;
uniform float uNoise;
uniform float uScanlines;
uniform float uChromaShift;

// Random function for noise
float random(vec2 co)
{
	return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

void main()
{
	vec2 uv = vTexCoord;
	
	// VHS tape wobble/distortion
	float wobble = sin(uv.y * 10.0 + uTime * 2.0) * uDistortion * 0.01;
	uv.x += wobble;
	
	// Horizontal lines/tracking errors
	float fractal = fract(uv.y * uScreenSize.y * 0.5 + uTime * 50.0);
	float lines = step(0.5, fractal);
	float trackingError = random(vec2(floor(uv.y * 10.0), floor(uTime * 3.0))) * lines * uDistortion * 0.05;
	uv.x += trackingError;
	
	// Chromatic aberration (VHS color shift)
	float shift = uChromaShift * 0.002;
	vec4 r = texture2D(gm_BaseTexture, vec2(uv.x + shift, uv.y));
	vec4 g = texture2D(gm_BaseTexture, uv);
	vec4 b = texture2D(gm_BaseTexture, vec2(uv.x - shift, uv.y));
	
	vec4 baseColor = vec4(r.r, g.g, b.b, g.a);
	
	// Scanlines
	float scanline = sin((uv.y * 90.0 + (uTime / 3.0) + (sin(uv.x * 10.0 + uTime * 2.0) * uDistortion * 0.01))) * 0.5 + 0.5;
	scanline = mix(1.0, scanline, uScanlines);
	baseColor.rgb *= scanline;
	
	// VHS noise/static
	float noise = random(vec2(uv.y * uTime, uTime)) * uNoise;
	baseColor.rgb += noise * 0.1;
	
	// Tape damage/corruption (random horizontal bars)
	float damage = step(0.998, random(vec2(uTime * 0.5, floor(uv.y * 100.0)))) * uNoise;
	baseColor.rgb = mix(baseColor.rgb, vec3(random(uv + uTime)), damage);
	
	// Color bleeding/saturation typical of VHS
	baseColor.rgb = clamp(baseColor.rgb, 0.0, 1.0);
	
	gl_FragColor = baseColor;
}