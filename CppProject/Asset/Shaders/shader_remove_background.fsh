uniform sampler2D uPrevSurface;
uniform sampler2D uAlphaSurface;

varying vec2 vTexCoord;

void main()
{
	vec4 BaseColor = texture2D(uPrevSurface, vTexCoord);
	float AlphaColor = texture2D(uAlphaSurface, vTexCoord).a;
	
	if (AlphaColor == 0.0)
		discard;
		
	gl_FragColor = vec4(BaseColor.rgb, AlphaColor);
}

