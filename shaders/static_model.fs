#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in float PartMask;

uniform sampler2D texture_diffuse1;
uniform int useTexture;
uniform int usePartColors;
uniform vec3 tintColor;
uniform vec3 partBaseColor;
uniform vec3 partAccentColor;
uniform vec3 lightDir;
uniform vec3 ambientColor;

void main()
{
    vec3 baseColor = tintColor;
    if (usePartColors == 1)
        baseColor = PartMask > 0.5 ? partAccentColor : partBaseColor;
    else if (useTexture == 1)
        baseColor = texture(texture_diffuse1, TexCoord).rgb * tintColor;

    vec3 n = normalize(Normal);
    float diffuse = max(dot(n, normalize(-lightDir)), 0.0);
    vec3 lighting = ambientColor + vec3(0.45) * diffuse;
    FragColor = vec4(baseColor * lighting, 1.0);
}
