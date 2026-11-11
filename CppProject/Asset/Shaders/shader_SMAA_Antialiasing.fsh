varying vec2 vTexCoord;

uniform sampler2D uScreen;
uniform vec2 uResolution;
uniform float uPower;

void main()
{
    vec2 texel = 1.0 / uResolution;

    // sample neighborhood
    vec4 cNW = texture2D(uScreen, vTexCoord + vec2(-1.0, -1.0) * texel);
    vec4 cNE = texture2D(uScreen, vTexCoord + vec2( 1.0, -1.0) * texel);
    vec4 cSW = texture2D(uScreen, vTexCoord + vec2(-1.0,  1.0) * texel);
    vec4 cSE = texture2D(uScreen, vTexCoord + vec2( 1.0,  1.0) * texel);
    vec4 cM  = texture2D(uScreen, vTexCoord);

    // extract rgb
    vec3 rgbNW = cNW.rgb;
    vec3 rgbNE = cNE.rgb;
    vec3 rgbSW = cSW.rgb;
    vec3 rgbSE = cSE.rgb;
    vec3 rgbM  = cM.rgb;

    // luma calculation (perceptual brightness)
    float lumaNW = dot(rgbNW, vec3(0.299, 0.587, 0.114));
    float lumaNE = dot(rgbNE, vec3(0.299, 0.587, 0.114));
    float lumaSW = dot(rgbSW, vec3(0.299, 0.587, 0.114));
    float lumaSE = dot(rgbSE, vec3(0.299, 0.587, 0.114));
    float lumaM  = dot(rgbM,  vec3(0.299, 0.587, 0.114));

    // find edge contrast
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
	
	vec4 result;

    // if contrast is low, skip AA (keeps sharpness)
    if (lumaMax - lumaMin < max(0.0312, lumaMax * 0.125 * uPower)) {
        result = cM; // keep original alpha
    } else {
	    // edge direction
	    vec2 dir;
	    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
	    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

	    // reduce length (avoid oversmoothing)
	    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * 1.0/8.0), 1.0/128.0);
	    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
	    dir = clamp(dir * rcpDirMin, -8.0, 8.0) * texel;

	    // blend samples along edge direction
	    vec4 cA = 0.5 * (
	        texture2D(uScreen, vTexCoord + dir * (1.0/3.0 - 0.5)) +
	        texture2D(uScreen, vTexCoord + dir * (2.0/3.0 - 0.5))
	    );

	    vec4 cB = cA * 0.5 + 0.25 * (
	        texture2D(uScreen, vTexCoord + dir * -0.5) +
	        texture2D(uScreen, vTexCoord + dir * 0.5)
	    );

	    float lumaB = dot(cB.rgb, vec3(0.299, 0.587, 0.114));
		result = ((lumaB < lumaMin) || (lumaB > lumaMax)) ? cA : cB;
	}

    gl_FragColor = result;
}