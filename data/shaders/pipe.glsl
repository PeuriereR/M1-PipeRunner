#version 330
#define M_PI 3.1415926535897932384626433832795

#ifdef VERTEX_SHADER

layout(location= 0) in vec3 position;
layout(location= 1) in vec2 texcoord;
layout(location= 2) in vec3 normal;

uniform mat4 mvpMatrix;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 viewInvMatrix;
uniform vec3 lightPosition;

out vec3 vertexNormal;
out vec3 toLightVector;
out vec3 toCameraVector;
out float visibility;
out vec3 vertexPosition;

const float density = 0.325;
const float gradient = 3;

void main(void){

    // Getting position
    gl_Position= mvpMatrix * vec4(position, 1);

    vec4 worldPosition = modelMatrix * vec4(position,1);

    // Getting vectors
    vertexNormal = (modelMatrix * vec4(normal, 0)).xyz;
    toLightVector = lightPosition.xyz - worldPosition.xyz;
    toCameraVector = (viewInvMatrix * vec4(0,0,0,1)).xyz - worldPosition.xyz;

    // Getting visibility for fog
    vec4 positionRelativeToCam = viewMatrix * worldPosition;
    float distance = length(positionRelativeToCam.xyz);
    visibility = exp(-pow((distance*density),gradient));
    visibility = clamp(visibility,0.0,1.0);

    // Vertex Position for cubemap reflection & refraction
    vertexPosition = worldPosition.xyz;
}

#endif

#ifdef FRAGMENT_SHADER

uniform vec4 mesh_color;
uniform samplerCube texture1;
uniform vec3 camera_position;
uniform int usecubemap;
uniform int difuselightstrength;

in vec3 vertexNormal;
in vec3 toLightVector;
in vec3 toCameraVector;
in vec3 vertexPosition;
in float visibility;

out vec4 out_Color;

void main(void){

    // BLINNPHONG lighting

    vec3 h = normalize(toCameraVector + toLightVector);

    float cos_thetab = dot(normalize(vertexNormal),normalize(toLightVector));
    float cos_theta_hb = dot(normalize(vertexNormal),h);

    int kb = difuselightstrength; // proportion difus //10
    int nb = 100; // concentration reflet
    // 10 OK renforcé pour texture cubemap sombre

    float fctb = kb / M_PI + (1- (1 - kb) * (nb + 1) / (2 * M_PI) * pow(cos_theta_hb,nb));
    // used later

    // B WALTER (Microfacet Models for Refraction through Rough Surfaces)

    // Getting our variables

    vec3 n = normalize(vertexNormal);
    vec3 i = normalize(toLightVector);
    vec3 o = reflect(-i, n);

    vec3 cam = normalize(toCameraVector);
    vec3 hr = normalize(i + o);

    float reflectionFactor = dot(o, cam); // used to mix refraction and reflection

    // F(i,hr)

    // nt different de ni et ni>nt pour "logique" matériau basique
    float nt = 0.5; //Index of refraction of media on the transmitted side //largeur lum
    // 1
    float ni = 1; //Index of refraction of the media on the incident side
    // 0.2
    float c = abs( dot(i,hr) );
    float g = sqrt( pow(nt,2) / pow(ni,2) - 1 + pow(c,2) );
    float f_i_hr = 1./2 * pow(g-c,2)/pow(g+c,2) * (1 + pow(c*(g+c) - 1,2)/pow(c*(g-c) + 1,2) );

    // G(i,o,hr)

    float ag = 0.1;  //0.2
    float g1_i_hr;

    if ( (dot(i,hr) / dot(i,n)) >0)
        g1_i_hr = 2. / (1 + sqrt(1 + pow(ag,2) * pow(tan(acos(dot(-cam,n))),2) ) ) ;
    else g1_i_hr = 0;

    float g1_o_hr;

    if ( (dot(o,hr) / dot(o,n)) >0)
        g1_o_hr = 2. / (1 + sqrt(1 + pow(ag,2) * pow(tan(acos(dot(-cam,n))),2) ) ) ;
    else g1_o_hr = 0;

    float g_i_o_hr = g1_i_hr * g1_o_hr;

    // D(hr)

    float rand_numb = fract(sin(dot(vec2(1,0) ,vec2(12.9898,78.233))) * 43758.5453);
    float ap = 100; // 45
    float teta_m = acos( pow(rand_numb,1./(ap+2)));
    float d_hr;
    if( dot(hr,n)>0 )
        d_hr = pow(ag,2) / (M_PI * pow(cos(teta_m),4) * pow(pow(ag,2) + pow(tan(teta_m),2),2) ) ;
    else d_hr = 0;

    float i_n = abs(dot(i,n));
    float o_n = abs(dot(o,n));
    int k = 2; // proportion difus
    float fct = ( (f_i_hr * g_i_o_hr * d_hr) / (4. * i_n * o_n ) ) ;
    float cos_theta = dot(n,i);
    fct = fct * max(cos_theta, 0.0) ;

    // Cubemap variables

    vec3 m = reflect(normalize(vertexPosition - camera_position),normalize(vertexNormal)); // reflexion
    vec3 m_refracted = refract(normalize(vertexPosition - camera_position),normalize(vertexNormal), 1.0);
    // 1/1.33 for usual refraction, 1 to see through

    // textures
    vec4 reflectedTexture = texture(texture1,m);
    vec4 refractedTexture = texture(texture1,m_refracted);

    // mixing to get refracted texture when 90° angle with the camera
    vec4 finaltex = mix(reflectedTexture, refractedTexture, reflectionFactor );
    finaltex = mix(finaltex, refractedTexture, 1-visibility);

    float blinnphong = fctb * max(cos_thetab, 0.0);
    float facteur_final = blinnphong;// mix(fct,blinnphong,0.5);
    if (fct>0.7)
        facteur_final = fct * blinnphong;
    facteur_final = max(0.7,facteur_final); // deleting dark parts
    out_Color = finaltex * facteur_final;

    out_Color = mix(refractedTexture , out_Color , visibility) ; // fog application
    if (usecubemap==1)
        out_Color = facteur_final * mesh_color;
}

#endif
