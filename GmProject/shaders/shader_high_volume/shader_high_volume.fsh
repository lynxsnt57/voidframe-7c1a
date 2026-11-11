// Bounded single-scattering volume. Each light is evaluated while its shadow
// atlas is current. The scene depth and light shadows use Mine-imator encoding.
#define PI 3.14159265359
uniform sampler2D uSceneDepth;
uniform sampler2D uSun0;
uniform sampler2D uSun1;
uniform sampler2D uSun2;
uniform sampler2D uLocalShadow;
uniform sampler2D uLightGobo;
uniform mat4 uInvProjection;
uniform mat4 uInvView;
uniform mat4 uCameraViewProjection;
uniform mat4 uVolumeInverse;
uniform mat4 uSunMatrix0;
uniform mat4 uSunMatrix1;
uniform mat4 uSunMatrix2;
uniform mat4 uSpotMatrix;
uniform mat4 uSpotShadowMatrix;
uniform vec3 uCameraPosition;
uniform vec3 uSunDirection;
uniform vec3 uLightPosition;
uniform vec3 uShadowPosition;
uniform vec4 uFogColor;
uniform vec4 uLightColor;
uniform vec3 uSunNear;
uniform vec3 uSunFar;
uniform vec3 uCascadeEnd;
uniform float uNear;
uniform float uFar;
uniform float uMaxDistance;
uniform float uLightNear;
uniform float uLightFar;
uniform float uLightStrength;
uniform float uLightFade;
uniform float uSpotSharpness;
uniform float uDensity;
uniform float uScattering;
uniform float uAbsorption;
uniform float uAnisotropy;
uniform float uEdgeSoftness;
uniform float uNoiseAmount;
uniform float uNoiseScale;
uniform float uSampleIndex;
uniform int uShape;
uniform int uPhase;
uniform int uGradient;
uniform int uNoiseWorld;
uniform vec4 uFogTopColor;
uniform vec4 uEmissionColor;
uniform float uEmission;
uniform float uNoiseDetail;
uniform float uNoiseContrast;
uniform float uNoiseThreshold;
uniform float uNoiseSoftness;
uniform float uNoiseSeed;
uniform vec3 uWindDirection;
uniform float uWindSpeed;
uniform float uWindInfluence;
uniform float uTurbulence;
uniform float uTurbulenceScale;
uniform float uTime;
uniform float uJitter;
uniform float uMinDistance;
uniform int uShadowSamples;
uniform float uShadowSoftness;
uniform float uSunTexel;
uniform vec2 uLocalTexel;
uniform int uSamples;
uniform int uPass;
uniform int uShadowEnabled;
uniform int uGoboMode;
uniform int uGoboTiling;
uniform int uGoboInvert;
uniform vec2 uGoboOffset;
uniform vec2 uGoboRepeat;
uniform float uGoboRotation;
uniform float uGoboIntensity;
uniform float uGoboFeather;
uniform float uGoboGamma;
varying vec2 vTexCoord;
float unpackDepth(vec4 c)
{ return dot(c.rgb, vec3(1.0, 0.003921569, 0.00001538)); }
float transformDepth(float d)
{ return (uFar - (uNear * uFar) / (d * (uFar-uNear) + uNear)) / (uFar-uNear); }
vec3 viewPosition(vec2 uv, float d)
{
    vec4 p = uInvProjection * vec4(uv.x*2.0-1.0, 1.0-uv.y*2.0, transformDepth(d), 1.0);
    return p.xyz / p.w;
}
float hash3(vec3 p)
{ return fract(sin(dot(p, vec3(12.9898,78.233,37.719))) * 43758.5453); }
float noise3(vec3 p)
{
    vec3 i=floor(p), f=fract(p); f=f*f*(vec3(3.0)-2.0*f);
    float a=mix(hash3(i),hash3(i+vec3(1,0,0)),f.x);
    float b=mix(hash3(i+vec3(0,1,0)),hash3(i+vec3(1,1,0)),f.x);
    float c=mix(hash3(i+vec3(0,0,1)),hash3(i+vec3(1,0,1)),f.x);
    float d=mix(hash3(i+vec3(0,1,1)),hash3(i+vec3(1,1,1)),f.x);
    return mix(mix(a,b,f.y),mix(c,d,f.y),f.z);
}
float shapeEdge(vec3 q)
{
    float box=min(min(1.0-abs(q.x),1.0-abs(q.y)),1.0-abs(q.z));
    if(uShape==1) return min(box,1.0-length(q));
    if(uShape==2) return min(box,min(1.0-length(q.xy),1.0-abs(q.z)));
    if(uShape==3) return min(box,min(1.0-abs(q.z),((1.0-q.z)*0.5-length(q.xy))*0.894427));
    if(uShape==4) { vec3 a=abs(q); float m=max((a.x+a.y+a.z)*0.577350269,max((a.y*0.618033989+a.z*1.618033989)*0.577350269,max((a.x*0.618033989+a.y*1.618033989)*0.577350269,(a.z*0.618033989+a.x*1.618033989)*0.577350269))); return min(box,0.794654472-m); }
    if(uShape==5) return min(box,0.3-length(vec2(length(q.xy)-0.7,q.z)));
    if(uShape==6) return min(box, 0.5-length(q-vec3(0.0,0.0,clamp(q.z,-0.5,0.5))));
    if(uShape==7) return min(box, (1.0-abs(q.x)-abs(q.y)-abs(q.z))*0.577350269);
    if(uShape==8) return min(box, 0.3-length(max(abs(q)-vec3(0.7),vec3(0.0))));
    return box;
}
float medium(vec3 p)
{
    vec3 local=(uVolumeInverse*vec4(p,1.0)).xyz/8.0;
    float e=shapeEdge(local);
    if(e<0.0) return 0.0;
    float edge=uEdgeSoftness>0.001 ? smoothstep(0.0,uEdgeSoftness,e) : 1.0;
    if(uNoiseAmount<=0.0) return edge;
    vec3 np=(uNoiseWorld!=0 ? p/8.0 : local)*max(0.01,uNoiseScale);
    np+=vec3(uNoiseSeed*0.017,uNoiseSeed*0.031,uNoiseSeed*0.047)-uWindDirection*uWindSpeed*uTime*uWindInfluence;
    if(uTurbulence>0.0) { vec3 t=np*max(0.01,uTurbulenceScale)+vec3(0.13,-0.09,0.07)*(uTime*uWindSpeed*uWindInfluence); np+=(vec3(noise3(t),noise3(t+vec3(17.0)),noise3(t+vec3(39.0)))-0.5)*uTurbulence; }
    float n=noise3(np);
    if(uNoiseDetail>0.0) n=mix(n,(n+0.5*noise3(np*2.0)+0.25*noise3(np*4.0))/1.75,clamp(uNoiseDetail,0.0,1.0));
    n=clamp((n-0.5)*max(0.0,uNoiseContrast)+0.5,0.0,1.0);
    if(uNoiseThreshold>0.0) n=smoothstep(uNoiseThreshold-max(0.001,uNoiseSoftness)*0.5,uNoiseThreshold+max(0.001,uNoiseSoftness)*0.5,n);
    return edge*mix(1.0,n*2.0,clamp(uNoiseAmount,0.0,1.0));
}
vec3 fogAt(vec3 p)
{
    if(uGradient==0) return uFogColor.rgb;
    vec3 q=(uVolumeInverse*vec4(p,1.0)).xyz/8.0;
    return mix(uFogColor.rgb,uFogTopColor.rgb,clamp(q.z*0.5+0.5,0.0,1.0));
}
vec2 shadowOffset(int i)
{
    float r=sqrt(float(i)/max(1.0,float(uShadowSamples-1)))*uShadowSoftness;
    return vec2(cos(float(i)*2.399963),sin(float(i)*2.399963))*r;
}
float sunShadow(vec3 p)
{
    vec4 clip=uCameraViewProjection*vec4(p,1.0);
    int c=0; if (clip.z>=uCascadeEnd.x) c=1; if (clip.z>=uCascadeEnd.y) c=2;
    vec4 sc; float nearZ,farZ;
    if(c==0) {sc=uSunMatrix0*vec4(p,1.0);nearZ=uSunNear.x;farZ=uSunFar.x;}
    else if(c==1) {sc=uSunMatrix1*vec4(p,1.0);nearZ=uSunNear.y;farZ=uSunFar.y;}
    else {sc=uSunMatrix2*vec4(p,1.0);nearZ=uSunNear.z;farZ=uSunFar.z;}
    if(sc.x<0.0||sc.y<0.0||sc.x>1.0||sc.y>1.0) return 1.0;
    float fragDepth=nearZ+sc.z*(farZ-nearZ);float visibility=0.0;
    for (int i=0;i<8;i++) { if(i>=uShadowSamples) break; vec2 uv=clamp(sc.xy+shadowOffset(i)*uSunTexel,0.0,1.0);float stored;
        if(c==0) stored=unpackDepth(texture2D(uSun0,uv)); else if(c==1) stored=unpackDepth(texture2D(uSun1,uv));else stored=unpackDepth(texture2D(uSun2,uv));
        float sampleDepth=nearZ+stored*(farZ-nearZ);visibility+=fragDepth-(1.0+float(c)*2.0)>sampleDepth ? 0.0 : 1.0;
    }
    return visibility/max(float(uShadowSamples),1.0);
}
vec2 pointCoord(vec3 position, vec3 look)
{
    vec3 u=vec3(-look.z*look.x,-look.z*look.y,1.0-look.z*look.z); u=normalize(u);
    vec3 v=cross(u,look); vec3 toPoint=(position-uShadowPosition)/dot(position-uShadowPosition,look);
    return vec2((dot(toPoint,v)+1.0)/6.0,(1.0-dot(toPoint,u))*0.25);
}
float pointShadow(vec3 p)
{
    vec3 d=p-uShadowPosition; vec3 ad=abs(d); vec3 look; vec2 base;
    if(ad.z>=ad.x && ad.z>=ad.y) {
        if(d.z>0.0) {look=normalize(vec3(0,-0.0001,1));base=vec2(1.0/3.0,0.5);}
        else {look=normalize(vec3(0,-0.0001,-1));base=vec2(2.0/3.0,0.5);}
    } else if(ad.x>=ad.y) {
        if(d.x>0.0) {look=vec3(1,0,0);base=vec2(0,0);}
        else {look=vec3(-1,0,0);base=vec2(1.0/3.0,0);}
    } else if(d.y>0.0) {look=vec3(0,1,0);base=vec2(2.0/3.0,0);}
    else {look=vec3(0,-1,0);base=vec2(0,0.5);}
    vec2 uv=pointCoord(p,look)+base;
    float visibility=0.0;
    for (int i=0;i<8;i++) {if(i>=uShadowSamples) break;
        vec2 sampleUV=clamp(uv+shadowOffset(i)*uLocalTexel,base+uLocalTexel*0.5,base+vec2(1.0/3.0,0.5)-uLocalTexel*0.5);
        float saved=uLightNear+unpackDepth(texture2D(uLocalShadow,sampleUV))*(uLightFar-uLightNear);
        visibility+=length(d)-0.8>saved ? 0.0 : 1.0;
    }
    return visibility/max(float(uShadowSamples),1.0);
}
float spotShadow(vec4 sh)
{
    vec2 uv=(vec2(sh.x,-sh.y)/sh.z+1.0)*0.5; float visibility=0.0;
    for (int i=0;i<8;i++) {if(i>=uShadowSamples) break;
        vec2 sampleUV=clamp(uv+shadowOffset(i)*uLocalTexel,0.0,1.0);
        float saved=uLightNear+unpackDepth(texture2D(uLocalShadow,sampleUV))*(uLightFar-uLightNear);
        visibility+=sh.z-1.0>saved ? 0.0 : 1.0;
    }
    return visibility/max(float(uShadowSamples),1.0);
}
vec3 spotCookie(vec4 sc)
{
    if(uGoboMode==0) return vec3(1.0);
    if(uGoboMode!=1 || sc.w<=0.0) return vec3(1.0-uGoboIntensity);
    vec2 uv=(vec2(sc.x,-sc.y)/sc.z)*0.5*uGoboRepeat;
    float co=cos(uGoboRotation),si=sin(uGoboRotation);
    uv=vec2(co*uv.x-si*uv.y,si*uv.x+co*uv.y)+vec2(0.5)+uGoboOffset*0.5*uGoboRepeat;
    float edge=1.0;
    if(uGoboTiling==0){float e=min(min(uv.x,uv.y),min(1.0-uv.x,1.0-uv.y));edge=step(0.0,e);if(uGoboFeather>0.0)edge*=smoothstep(0.0,max(0.0001,uGoboFeather*0.5),e);uv=clamp(uv,0.0,1.0);}
    vec4 tex=texture2D(uLightGobo,uv); vec3 col=tex.rgb;
    if(uGoboInvert!=0) col=vec3(1.0)-col;
    return mix(vec3(1.0),pow(max(col,vec3(0.0)),vec3(max(uGoboGamma,0.01)))*tex.a*edge,clamp(uGoboIntensity,0.0,1.0));
}
float phaseHG(float g,float cost)
{
    float denom=max(0.001,1.0+g*g-2.0*g*cost);
    return (1.0-g*g)/(4.0*PI*denom*sqrt(denom));
}
vec3 lightAt(vec3 p,vec3 ray)
{
    vec3 toLight; float attenuation=1.0; vec3 cookie=vec3(1.0); float visibility=1.0;
    if(uPass==1){toLight=normalize(uSunDirection);if(uShadowEnabled!=0)visibility=sunShadow(p);}
    else {
        vec3 delta=uLightPosition-p; float distance=length(delta);
        if(distance<0.001||distance>=uLightFar) return vec3(0.0);
        toLight=delta/distance;
        attenuation=1.0-clamp((distance-uLightFar*(1.0-uLightFade))/(uLightFar*max(0.0001,uLightFade)),0.0,1.0);
        if(uPass==2) {if(uShadowEnabled!=0)visibility=pointShadow(p);}
        if(uPass==3){
            vec4 sc=uSpotMatrix*vec4(p,1.0);
            if(sc.z<=0.0||sc.w<=0.0)return vec3(0.0);
            vec2 uv=(vec2(sc.x,-sc.y)/sc.z+1.0)*0.5;
            if(uv.x<=0.0||uv.y<=0.0||uv.x>=1.0||uv.y>=1.0)return vec3(0.0);
            attenuation*=1.0-clamp((length(uv-vec2(0.5))-0.5*uSpotSharpness)/(0.5*max(0.01,1.0-uSpotSharpness)),0.0,1.0);
            if(uShadowEnabled!=0){vec4 sh=uSpotShadowMatrix*vec4(p,1.0);visibility=spotShadow(sh);}
            cookie=spotCookie(sc);
        }
    }
    float g=clamp(uAnisotropy,0.0,0.95);float cost=dot(ray,toLight);
    float phase=phaseHG(g,cost);
    if(uPhase==1) {float exponent=g*16.0/(1.05-g);phase=(exponent+1.0)/(4.0*PI)*pow(clamp((1.0+cost)*0.5,0.0,1.0),exponent);}
    if(uPhase==2) phase=0.8*phase+0.2*phaseHG(-g*0.5,cost);
    return uLightColor.rgb*cookie*uLightStrength*attenuation*visibility*phase;
}
void main()
{
    float depth=unpackDepth(texture2D(uSceneDepth,vTexCoord));
    float valid=(depth>0.000001 && depth<0.999999) ? 1.0:0.0;
    vec3 viewFar=viewPosition(vTexCoord,valid>0.5 ? depth : 0.99999);
    vec3 worldEnd=(uInvView*vec4(viewFar,1.0)).xyz;
    vec3 ray=normalize(worldEnd-uCameraPosition);
    float limit=valid>0.5 ? min(length(worldEnd-uCameraPosition),uMaxDistance) : uMaxDistance;
    vec3 o=(uVolumeInverse*vec4(uCameraPosition,1.0)).xyz;
    vec3 d=(uVolumeInverse*vec4(ray,0.0)).xyz;
    vec3 safeD=vec3(abs(d.x)<0.000001 ? (d.x<0.0?-0.000001:0.000001) : d.x,abs(d.y)<0.000001 ? (d.y<0.0?-0.000001:0.000001) : d.y,abs(d.z)<0.000001 ? (d.z<0.0?-0.000001:0.000001) : d.z);
    vec3 t0=(-vec3(8.0)-o)/safeD, t1=(vec3(8.0)-o)/safeD;
    vec3 mn=min(t0,t1),mx=max(t0,t1);float start=max(uMinDistance,max(mn.x,max(mn.y,mn.z)));float stop=min(limit,min(mx.x,min(mx.y,mx.z)));
    if(stop<=start || uDensity<=0.0) { gl_FragColor=vec4(0.0); }
    else
    {
    float stepSize=(stop-start)/float(uSamples);
    float jitter=fract(sin(dot(floor(vTexCoord*8192.0)+vec2(uSampleIndex),vec2(12.9898,78.233)))*43758.5453);
    jitter=mix(0.5,jitter,clamp(uJitter,0.0,1.0));
    float trans=1.0; vec3 result=vec3(0.0);
    for(int i=0;i<128;i++){
        if(i>=uSamples) break;
        vec3 p=uCameraPosition+ray*(start+(float(i)+jitter)*stepSize);
        float density=max(0.0,uDensity)*0.008*medium(p);
        float sigma=density*(1.0+max(0.0,uAbsorption));
        float nextT=exp(-sigma*stepSize);
        float weight=sigma>0.000001 ? (1.0-nextT)/sigma : stepSize;
        if(uPass==0) result+=trans*(1.0-nextT)*(fogAt(p)*uLightColor.rgb*uLightStrength+uEmissionColor.rgb*max(0.0,uEmission));
        else result+=trans*density*max(0.0,uScattering)*weight*lightAt(p,ray)*fogAt(p);
        trans*=nextT;
    }
    if(uPass==0) gl_FragColor=vec4(result/max(1.0-trans,0.000001),1.0-trans);
    else gl_FragColor=vec4(result,1.0);
    }
}
