uniform sampler2D uTexture; // static

uniform float uSampleIndex;
uniform int uAlphaHash;

uniform int uColorsExt;
uniform vec4 uRGBAdd;
uniform vec4 uRGBSub;
uniform vec4 uHSBAdd;
uniform vec4 uHSBSub;
uniform vec4 uHSBMul;
uniform vec4 uMixColor;

uniform int uFogShow;
uniform int uFogHeightShow;
uniform vec4 uFogColor; // static
uniform float uFogDistance; // static
uniform float uFogSize; // static
uniform float uFogHeight; // static
uniform float uFogHeightSize; // static
uniform float uFogHeightOffset; // static
uniform vec4 uFogHeightColor; // static

uniform vec3 uCameraPosition; // static

varying vec3 vPosition;
varying float vDepth;
varying vec4 vColor;
varying vec2 vTexCoord;

vec4 rgbtohsb(vec4 c)
{
	vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
	vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
	vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

	float d = q.x - min(q.w, q.y);
	float e = 1.0e-10;
	return vec4(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x, c.a);
}

vec4 hsbtorgb(vec4 c)
{
	vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
	vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
	return vec4(c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y), c.a);
}

vec2 getFog()
{
    float fog1 = 0.0;
    float fog2 = 0.0;

    if (uFogShow > 0)
    {
        float fogDepth = distance(vPosition, uCameraPosition);

        // Main fog
        fog1 = clamp(1.0 - (uFogDistance - fogDepth) / uFogSize, 0.0, 1.0);
        fog1 *= clamp(1.0 - (vPosition.z - uFogHeight) / uFogSize, 0.0, 1.0);

        // Height fog
        if (uFogHeightShow > 0)
        {
            fog2 = clamp(1.0 - (0.0 - fogDepth) / uFogHeightSize, 0.0, 1.0);
            fog2 *= clamp(1.0 - (vPosition.z - uFogHeightOffset) / uFogHeightSize, 0.0, 1.0);
        }
    }

    return vec2(fog1, fog2);
}

float hash(vec2 c)
{
	return fract(10000.0 * sin(17.0 * c.x + 0.1 * c.y) *
	(0.1 + abs(sin(13.0 * c.y + c.x))));
}

void main()
{
	vec2 tex = vTexCoord;
	vec4 baseColor = vColor * texture2D(uTexture, tex); // Get base
	
	if (uColorsExt > 0)
	{
		gl_FragColor = clamp(baseColor + uRGBAdd - uRGBSub, 0.0, 1.0); // Transform RGB
		gl_FragColor = hsbtorgb(clamp(rgbtohsb(gl_FragColor) + uHSBAdd - uHSBSub, 0.0, 1.0) * uHSBMul); // Transform HSB
		gl_FragColor = mix(gl_FragColor, uMixColor, uMixColor.a); // Mix
		
		vec2 fog = getFog();

		gl_FragColor = mix(gl_FragColor, uFogHeightColor, fog.y);
		gl_FragColor = mix(gl_FragColor, uFogColor, fog.x);

		gl_FragColor.a = baseColor.a; // Correct alpha
	}
	else 
	{
		gl_FragColor = baseColor;
		
		vec2 fog = getFog();

		gl_FragColor = mix(gl_FragColor, uFogHeightColor, fog.y);
		gl_FragColor = mix(gl_FragColor, uFogColor, fog.x);

		gl_FragColor.a = baseColor.a; // Correct alpha
	}
	
	if (uAlphaHash > 0)
	{
		if (gl_FragColor.a < hash(vec2(hash(vPosition.xy + (uSampleIndex / 255.0)), vPosition.z + (uSampleIndex / 255.0))))
			discard;
		else
			gl_FragColor.a = 1.0;
	}
	
	if (gl_FragColor.a == 0.0)
		discard;
}
