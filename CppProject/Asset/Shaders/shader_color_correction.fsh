varying vec2 vTexCoord;

uniform float uContrast;
uniform float uBrightness;
uniform float uSaturation;
uniform float uVibrance;
uniform bool uInvert;
uniform float uInvertBlend;
uniform float uHue;
uniform vec4 uColorBurn;

vec4 rgbtohsb(vec4 c)
{
	vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
	vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
	vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

	float d = q.x - min(q.w, q.y);
	float e = 1.0e-10;
	return vec4(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x, c.a);
}

vec3 hueRotateYIQ(vec3 c, float angle)
{
	// RGB to YIQ (scalar form)
	float Y = dot(c, vec3(0.299,  0.587,  0.114));
	float I = dot(c, vec3(0.596, -0.274, -0.322));
	float Q = dot(c, vec3(0.211, -0.523,  0.312));

	// Rotate chroma (I/Q)
	float cosA = cos(angle);
	float sinA = sin(angle);

	float I2 = I * cosA - Q * sinA;
	float Q2 = I * sinA + Q * cosA;

	// YIQ to RGB (scalar form)
	vec3 rgb;
	rgb.r = Y + 0.956 * I2 + 0.621 * Q2;
	rgb.g = Y - 0.272 * I2 - 0.647 * Q2;
	rgb.b = Y - 1.107 * I2 + 1.704 * Q2;

	return clamp(rgb, 0.0, 1.0);
}

void main()
{
	// Get base
	vec4 baseColor = texture2D(gm_BaseTexture, vTexCoord);
	
	// Brightness and contrast
	baseColor.rgb = (baseColor.rgb - vec3(0.5)) * vec3(uContrast) + vec3(uBrightness + 0.5);
	baseColor.rgb = clamp(baseColor.rgb, vec3(0.0), vec3(1.0));
	
	// Saturation
	vec3 W = vec3(0.2125, 0.7154, 0.0721);
	vec3 satIntensity = vec3(dot(baseColor.rgb, W));
	baseColor.rgb = mix(satIntensity, baseColor.rgb, uSaturation);
	baseColor.rgb = clamp(baseColor.rgb, vec3(0.0), vec3(1.0));
	
	// Vibrance(Saturates desaturated colors)
	satIntensity = vec3(dot(baseColor.rgb, W));
	float sat = rgbtohsb(baseColor).g;
	float vibrance = 1.0 - pow(pow(sat, 8.0), .15);
	baseColor.rgb = mix(satIntensity, baseColor.rgb, 1.0 + (vibrance * uVibrance));
	baseColor.rgb = clamp(baseColor.rgb, vec3(0.0), vec3(1.0));
	
	if (abs(uHue) >= 0.01)
		baseColor.rgb = hueRotateYIQ(baseColor.rgb, radians(uHue));
	
	if (uInvert)
	{
		vec4 InvertedBase = vec4(1.0 - baseColor.rgb, baseColor.a);
		baseColor = mix(baseColor, InvertedBase, uInvertBlend);
	}
	
	// Color burn
	baseColor.rgb = 1.0 - (1.0 - baseColor.rgb) / uColorBurn.rgb; 
	
	gl_FragColor = baseColor;
}