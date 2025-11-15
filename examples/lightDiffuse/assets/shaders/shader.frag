#version 450

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;

layout(binding = 1) uniform sampler2D texSampler;

layout(binding = 2) uniform LightingUBO {
    vec3 lightPos;
    float _pad1;
    vec3 lightColor;
    float intensity;
    vec3 viewPos;
    float _pad2;
    vec3 ambientColor;
    float ambientIntensity;
} lighting;

layout(location = 0) out vec4 outColor;

void main() {
    // Sample the texture
    vec4 texColor = texture(texSampler, fragTexCoord);
    
    // Normalize the interpolated normal
    vec3 norm = normalize(fragNormal);
    
    // Ambient lighting
    vec3 ambient = lighting.ambientColor * lighting.ambientIntensity;
    
    // Diffuse lighting
    vec3 lightDir = normalize(lighting.lightPos - fragWorldPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lighting.lightColor * lighting.intensity;
    
    // Specular lighting (Blinn-Phong)
    vec3 viewDir = normalize(lighting.viewPos - fragWorldPos);
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfDir), 0.0), 32.0);
    vec3 specular = spec * lighting.lightColor * lighting.intensity * 0.5;
    
    // Combine everything
    vec3 result = (ambient + diffuse + specular) * texColor.rgb;
    
    outColor = vec4(result, texColor.a);
}
