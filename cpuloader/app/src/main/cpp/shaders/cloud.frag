#version 300 es
precision highp float;
in vec2 vTexCoord;
out vec4 fragColor;

// 改进的3D噪声函数
float hash(float n) { return fract(sin(n)*753.5453123); }
float noise(vec3 x) {
    vec3 p = floor(x);
    vec3 f = fract(x);
    f = f*f*(3.0-2.0*f);
    
    float n = p.x + p.y*157.0 + 113.0*p.z;
    return mix(mix(mix(hash(n+0.0), hash(n+1.0),f.x),
               mix(hash(n+157.0), hash(n+158.0),f.x),f.y),
           mix(mix(hash(n+113.0), hash(n+114.0),f.x),
               mix(hash(n+270.0), hash(n+271.0),f.x),f.y),f.z);
}

// 复杂的光线步进函数
vec4 raymarch(vec3 ro, vec3 rd) {
    float t = 0.0;
    vec3 col = vec3(0.0);
    
    for(int i=0; i<256; i++) {
        vec3 p = ro + rd*t;
        
        // 多层噪声叠加
        float n = noise(p*0.5)*0.5;
        n += noise(p*1.0)*0.25;
        n += noise(p*2.0)*0.125;
        
        // 密度场
        float dens = 0.5 - length(p*0.2) + n;
        
        if(dens > 0.01) {
            // 光照计算
            vec3 lightPos = vec3(2.0, 3.0, -1.0);
            vec3 lightDir = normalize(lightPos - p);
            float diff = max(dot(normalize(p), lightDir), 0.0);
            float shadow = 1.0;
            
            // 阴影计算
            vec3 sp = p + lightDir*0.1;
            for(int j=0; j<16; j++) {
                float s = noise(sp*0.5);
                if(s > 0.3) {
                    shadow = 0.2;
                    break;
                }
                sp += lightDir*0.1;
            }
            
            col += vec3(1.0, 0.9, 0.8) * diff * shadow * 0.1;
            t += 0.02;
        } else {
            t += 0.1;
        }
        
        if(t > 50.0) break;
    }
    
    return vec4(col, 1.0);
}

void main() {
    vec3 ro = vec3(0.0, 0.0, -10.0);
    vec2 uv = vTexCoord - 0.5;
    vec3 rd = normalize(vec3(uv, 1.0));
    
    // 添加相机旋转
    float time = float(gl_FragCoord.x) * 0.01;
    ro.xz *= mat2(cos(time), -sin(time), sin(time), cos(time));
    rd.xz *= mat2(cos(time), -sin(time), sin(time), cos(time));
    
    fragColor = raymarch(ro, rd);
}
