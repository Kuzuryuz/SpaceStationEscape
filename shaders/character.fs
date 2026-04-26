#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform sampler2D diffuseTexture;
uniform vec3 lightDir;
uniform vec3 tintColor;
uniform vec3 solidColor;
uniform int useSolidColor;

void main()
{
    vec3 albedo = useSolidColor == 1
        ? solidColor
        : texture(diffuseTexture, TexCoord).rgb * tintColor;
    vec3 n = normalize(Normal);
    float diffuse = max(dot(n, normalize(-lightDir)), 0.0);
    vec3 lighting = useSolidColor == 1
        ? vec3(0.72, 0.70, 0.82) + vec3(0.45) * diffuse
        : vec3(max(diffuse, 0.2));
    vec3 color = albedo * lighting;
    FragColor = vec4(color, 1.0);
}
