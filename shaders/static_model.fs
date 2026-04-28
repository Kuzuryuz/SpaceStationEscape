#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in float PartMask;
in vec3 MaterialColor;

uniform sampler2D texture_diffuse1;
uniform int useTexture;
uniform int usePartColors;
uniform int recolorBlueToRed;
uniform vec3 tintColor;
uniform vec3 partBaseColor;
uniform vec3 partAccentColor;
uniform vec3 lightDir;
uniform vec3 ambientColor;

void main()
{
    vec3 baseColor = MaterialColor * tintColor;
    if (usePartColors == 1)
        baseColor = PartMask > 0.5 ? partAccentColor : partBaseColor;
    else if (useTexture == 1)
    {
        baseColor = texture(texture_diffuse1, TexCoord).rgb * tintColor;
        if (recolorBlueToRed == 1 &&
            baseColor.b > 0.35 &&
            baseColor.b > baseColor.r * 1.25 &&
            baseColor.b > baseColor.g * 1.08)
        {
            float value = max(max(baseColor.r, baseColor.g), baseColor.b);
            baseColor = vec3(value, value * 0.16, value * 0.13);
        }
    }

    vec3 n = normalize(Normal);
    float diffuse = max(dot(n, normalize(-lightDir)), 0.0);
    vec3 lighting = ambientColor + vec3(0.45) * diffuse;
    FragColor = vec4(baseColor * lighting, 1.0);
}
