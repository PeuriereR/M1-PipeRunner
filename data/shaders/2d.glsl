#version 330
#define M_PI 3.1415926535897932384626433832795

#ifdef VERTEX_SHADER

layout(location= 0) in vec3 position;
layout(location= 1) in vec2 texcoord;

uniform mat4 mvpMatrix;
out vec2 vertex_texcoord;

void main(void){

    gl_Position= vec4(position.xy,-1,1);

    vertex_texcoord= texcoord;
}

#endif

#ifdef FRAGMENT_SHADER

uniform sampler2D texture0;
in vec2 vertex_texcoord;

out vec4 out_Color;

void main(void){

    out_Color = texture(texture0, vertex_texcoord);
    //    if(out_Color.x==0 && out_Color.y==0 && out_Color.z==0){
    //        discard;
        //}
}

#endif
