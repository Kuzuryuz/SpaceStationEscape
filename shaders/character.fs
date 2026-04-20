#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform sampler2D diffuseTexture;
uniform vec3 lightDir;
uniform vec3 tintColor;

void main()
{
    vec3 albedo = texture(diffuseTexture, TexCoord).rgb * tintColor;
    vec3 n = normalize(Normal);
    float diffuse = max(dot(n, normalize(-lightDir)), 0.2);
    vec3 color = albedo * diffuse;
    FragColor = vec4(color, 1.0);
}
