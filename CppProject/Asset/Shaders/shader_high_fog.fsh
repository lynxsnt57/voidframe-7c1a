uniform sampler2D uTexture; // static

uniform float uSampleIndex;
uniform int uAlphaHash;

uniform int uFogShow;
uniform int uFogHeightShow;
uniform vec4 uFogColor; // static
uniform float uFogDistance; // static
uniform float uFogSize; // static
uniform float uFogHeight; // static
uniform float uFogHeightSize; // static
uniform float uFogHeightOffset; // static

uniform vec3 uCameraPosition; // static

varying vec3 vPosition;
varying vec4 vColor;
varying vec2 vTexCoord;
varying float vDepth;

float getFog(int check)
{
	float fog, fog2;
	fog = 0.0;
	fog2 = 0.0;
	
	if (uFogShow > 0)
	{
		float fogDepth = distance(vPosition, uCameraPosition);
		
		if (check < 1)
		{
			fog = clamp(1.0 - (uFogDistance - fogDepth) / uFogSize, 0.0, 1.0);
			fog *= clamp(1.0 - (vPosition.z - uFogHeight) / uFogSize, 0.0, 1.0);
		} else
		{
			if (uFogHeightShow > 0)
			{
				fog2 = clamp(1.0 - (0.0 - fogDepth) / uFogHeightSize, 0.0, 1.0);
				fog2 *= clamp(1.0 - (vPosition.z - uFogHeightOffset) / uFogHeightSize, 0.0, 1.0);
				fog += fog2;
			} 
		}
	}
	
	return fog;
}

float hash(vec2 c)
{
	return fract(10000.0 * sin(17.0 * c.x + 0.1 * c.y) *
	(0.1 + abs(sin(13.0 * c.y + c.x))));
}

void main()
{
	vec2 tex = vTexCoord;
	vec4 baseColor = vColor * texture2D(uTexture, tex);
	
	if (baseColor.a > 0.0) {
		gl_FragData[0] = vec4(vec3(getFog(0)), 1.0);
		gl_FragData[1] = vec4(vec3(getFog(1)), 1.0);
	}
	else
		discard;
	
	if (uAlphaHash > 0 && (baseColor.a < hash(vec2(hash(vPosition.xy + (uSampleIndex / 255.0)), vPosition.z + (uSampleIndex / 255.0)))))
		discard;
}
