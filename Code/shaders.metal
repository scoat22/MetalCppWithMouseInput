#include <metal_stdlib>
using namespace metal;

struct v2f
{
    float4 position [[position]];
    float3 normal;
    half3 color;
};

struct VertexData
{
    float3 position;
    float3 normal;
};

struct InstanceData
{
    float4x4 instanceTransform;
    float3x3 instanceNormalTransform;
    float4 instanceColor;
};

struct CameraData
{
    float4x4 perspectiveTransform;
    float4x4 worldTransform;
    float3x3 worldNormalTransform;
};

v2f vertex vert(device const VertexData* vertexData [[buffer(0)]],
                device const InstanceData* instanceData [[buffer(1)]],
                device const CameraData& cameraData [[buffer(2)]],
                uint vertexId [[vertex_id]],
                uint instanceId [[instance_id]] )
{
    v2f o;

    const device VertexData& vd = vertexData[ vertexId ];
    float4 pos = float4( vd.position, 1.0 );
    pos = instanceData[ instanceId ].instanceTransform * pos;
    pos = cameraData.perspectiveTransform * cameraData.worldTransform * pos;
    o.position = pos;

    float3 normal = instanceData[ instanceId ].instanceNormalTransform * vd.normal;
    normal = cameraData.worldNormalTransform * normal;
    o.normal = normal;

    o.color = half3( instanceData[ instanceId ].instanceColor.rgb );
    return o;
}

half4 fragment frag(v2f i [[stage_in]])
{
    // assume light coming from (front-top-right)
    float3 L = normalize(float3( 1.0, 1.0, 0.8 ));
    float3 N = normalize(i.normal);

    float ndotl = saturate(dot(N, L));
    return half4(i.color * 0.1 + i.color * ndotl, 1.0 );
}
