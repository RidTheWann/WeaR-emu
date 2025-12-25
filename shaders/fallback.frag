#version 450

/**
 * @file fallback.frag
 * @brief Fallback fragment shader for untranslated PS4 geometry
 * 
 * Renders geometry in Neon Magenta to clearly indicate debug/fallback mode.
 */

// Input from vertex shader
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in float fragDepth;

// Push constants
layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec4 debugColor;    // User-specified debug color
    float time;
    float padding[3];
} pc;

// Output color
layout(location = 0) out vec4 outColor;

// Neon Magenta/Pink for "FALLBACK MODE" visibility
const vec3 NEON_MAGENTA = vec3(1.0, 0.0, 0.6);
const vec3 NEON_CYAN    = vec3(0.0, 1.0, 0.8);
const vec3 HOT_PINK     = vec3(1.0, 0.2, 0.6);

void main() {
    // Basic lighting (simple lambert)
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float ndotl = max(dot(fragNormal, lightDir), 0.0);
    float ambient = 0.2;
    float lighting = ambient + (1.0 - ambient) * ndotl;
    
    // Choose base color
    vec3 baseColor;
    if (pc.debugColor.a > 0.0) {
        // Use user-specified debug color
        baseColor = pc.debugColor.rgb;
    } else {
        // Default: Neon Magenta with normal-based variation
        baseColor = mix(NEON_MAGENTA, HOT_PINK, fragNormal.y * 0.5 + 0.5);
    }
    
    // Apply vertex color for variation
    vec3 finalColor = baseColor * fragColor * lighting;
    
    // Add depth-based fog for visual depth cue
    float fogFactor = clamp(fragDepth * 0.5, 0.0, 0.8);
    finalColor = mix(finalColor, vec3(0.02, 0.02, 0.03), fogFactor);
    
    // Edge highlight based on normal facing (fresnel-like effect)
    float edgeFactor = 1.0 - abs(dot(fragNormal, vec3(0.0, 0.0, 1.0)));
    finalColor += NEON_CYAN * edgeFactor * 0.3;
    
    // Pulsing emission for "debug mode" visibility
    float pulse = sin(pc.time * 3.0) * 0.1 + 0.9;
    finalColor *= pulse;
    
    // Scanline effect (optional retro look)
    float scanline = sin(gl_FragCoord.y * 2.0) * 0.03 + 0.97;
    finalColor *= scanline;
    
    outColor = vec4(finalColor, 1.0);
}
