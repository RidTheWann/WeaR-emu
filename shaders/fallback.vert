#version 450

/**
 * @file fallback.vert
 * @brief Fallback vertex shader for untranslated PS4 geometry
 * 
 * Displays geometry in debug colors when PS4 shaders are not yet recompiled.
 */

// Vertex input (matches common PS4 vertex formats)
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

// Push constants for transformation
layout(push_constant) uniform PushConstants {
    mat4 mvp;           // Model-View-Projection matrix
    vec4 debugColor;    // Debug color (RGBA)
    float time;         // Animation time
    float padding[3];
} pc;

// Output to fragment shader
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out float fragDepth;

void main() {
    // Transform vertex position
    vec4 worldPos = pc.mvp * vec4(inPosition, 1.0);
    gl_Position = worldPos;
    
    // Pass data to fragment shader
    fragNormal = normalize(inNormal);
    fragTexCoord = inTexCoord;
    fragDepth = worldPos.z / worldPos.w;
    
    // Generate debug color based on vertex position and normal
    // This creates a distinctive "debug" look
    vec3 normalColor = (inNormal + 1.0) * 0.5;  // Map -1,1 to 0,1
    vec3 posColor = fract(inPosition * 0.1);    // Position-based pattern
    
    // Mix between normal-based and position-based coloring
    fragColor = mix(normalColor, posColor, 0.3);
    
    // Add time-based pulsing for visibility
    float pulse = sin(pc.time * 2.0) * 0.1 + 0.9;
    fragColor *= pulse;
}
