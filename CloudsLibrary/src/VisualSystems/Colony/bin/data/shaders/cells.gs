#version 120
#extension all : warn
#define RESOLUTION 3
#define PI 3.14159265359
#define SPREAD_RADIUS 100.


uniform vec2 screenResolution;

float hash(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

void main() {
    //IN
    vec3 p_in = gl_PositionIn[0].xyz;
    vec2 data = gl_PositionIn[1].xy;     //size = data.x, angle = data.y;
    
    //FLAT PLACEMENT
    vec4 p = vec4(vec3(p_in.xy - screenResolution/2.,100. + (p_in.x + p_in.y)*0.01),1.);
    
    //POINT GENERATION
    
for (int iterations = 0 ; iterations < 2 ; iterations ++){
    float opacity = 1. / pow(100., iterations);
    float magnitude = 1. + iterations * 50.;
    vec4 zShift = vec4(0.,0.,iterations * 4. ,0.);
    for (int i = 0; i < RESOLUTION ; i++) {
        /*
         b
         |\
         | \
         a--c
         */
            float f = data.y + (float(i) / float(RESOLUTION)) * 2. * PI;
        
            //a
            gl_Position = gl_ModelViewProjectionMatrix  * (p
                                                           + vec4(data.x * magnitude * vec3(cos(f), sin(f), 0.), 0.)
                                                           + zShift);
            gl_FrontColor = vec4(1., 1., 1., opacity);
            EmitVertex();
            
            //b
            gl_Position =  gl_ModelViewProjectionMatrix * (p + zShift);
            gl_FrontColor = vec4(1., 1., 1., 0.2);
            EmitVertex();
            
            //c
            f += (1. / float(RESOLUTION)) * 2. * PI;
            gl_Position =  gl_ModelViewProjectionMatrix * (p
                                                           + vec4(data.x * magnitude * vec3(cos(f), sin(f), 0.), 0.)
                                                           + zShift);
            gl_FrontColor = vec4(1., 1., 1., opacity);
            EmitVertex();
            EndPrimitive();
        }
    }

}