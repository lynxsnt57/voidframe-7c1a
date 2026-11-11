uniform sampler2D uAddTexture;
uniform vec4 uBlendColor;
uniform float uAmount;
uniform float uPower;
uniform int uBlendMode;

varying vec2 vTexCoord;

vec3 blendAdd(vec3 base, vec3 blend)
{
    return base + blend;
}

vec3 blendMultiply(vec3 base, vec3 blend)
{
    return base * blend;
}

vec3 blendScreen(vec3 base, vec3 blend)
{
    return 1.0 - (1.0 - base) * (1.0 - blend);
}

vec3 blendOverlay(vec3 base, vec3 blend)
{
    return mix(
        2.0 * base * blend,
        1.0 - 2.0 * (1.0 - base) * (1.0 - blend),
        step(0.5, base)
    );
}

void main()
{
    vec4 baseColor = texture2D(gm_BaseTexture, vTexCoord);
    vec4 addColor  = texture2D(uAddTexture, vTexCoord);

    // Power curve
    addColor.rgb = pow(addColor.rgb, vec3(uPower));

    // Luminance-based alpha
    float value = (addColor.r + addColor.g + addColor.b) / 3.0;
    addColor.a = value;

    vec3 blended;

    vec3 blendInput = (addColor.rgb * uBlendColor.rgb) * uAmount;

    if (uBlendMode == 1)
        blended = blendAdd(baseColor.rgb, blendInput);
    else if (uBlendMode == 2)
        blended = blendScreen(baseColor.rgb, blendInput);
    else if (uBlendMode == 3)
        blended = blendMultiply(baseColor.rgb, blendInput);
    else if (uBlendMode == 4)
        blended = blendOverlay(baseColor.rgb, blendInput);
    else
        blended = mix(baseColor.rgb, addColor.rgb, addColor.a * uAmount);

    vec4 finalColor;
    finalColor.rgb = blended;
    finalColor.a   = min(baseColor.a + addColor.a, 1.0);

    gl_FragColor = finalColor;
}