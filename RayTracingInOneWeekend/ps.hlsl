struct ps_input {
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
    float4 world : POSITION;
};

cbuffer world_constants : register(b0) {
    float gScreenWidth;
    float gScreenHeight;
    float2 gWorldPadding;
};

cbuffer frame_constants : register(b1) {
    float4x4 gViewProjectionMatrix;
    float4x4 gViewMatrix;
    float4x4 gProjectionMatrix;
    float gGameTime;
    float gGameFrameTime;
    float gSamplesPerPixel;
    float gMaxDepth;
    float3 gEyePosition;
    float3 gLookAt;
    float gVFovRadians;
    float gAspectRatio;
    float gAperture;
    float gLensRadius;
    float gFocusDistance;
    float gFramePadding;
};

cbuffer object_constants : register(b2) {
    float4x4 gModelMatrix;
    float4 gMaterialColor;
    float3 gMaterialAttenuation;
    float gSphereRadius;
    float gMaterialType;
    float gMaterialRoughness;
    float gMaterialMetallic;
    float gMaterialRefractionIndex;
};

SamplerState sSampler : register(s0);

Texture1D<float4> tRandomUV : register(t0);
Texture2D<float4> tRandomDiskResult : register(t1);

struct ray {
    float3 position;
    float3 direction;
};

struct Material {
    float4 color;
    float3 attenuation;
    float type;
    float roughness;
    float metallic;
    float refractionIndex;
};

struct hit_record {
    float3 p;
    float3 normal;
    Material material;
    float t;
    bool hit;
    bool front_face;
};

struct Sphere {
    float3 center;
    float radius;
    Material material;
};

#define MAX_SPHERES 512
Sphere spheres[MAX_SPHERES];

bool near_zero(float3 vec) {
    const float epsilon = 0.000000001f;
    return (abs(vec.x) < epsilon) && ((vec.y) < epsilon) && (abs(vec.z) < epsilon);
}

bool near_zero(float2 vec) {
    const float epsilon = 0.000000001f;
    return (abs(vec.x) < epsilon) && ((vec.y) < epsilon);
}

bool near_zero(float scalar) {
    const float epsilon = 0.000000001f;
    return (abs(scalar) < epsilon);
}

float nrand(float2 uv) {
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

void set_face_normal(in ray r, in float3 outward_normal, out bool front_face, out float3 normal) {
    front_face = dot(r.direction, outward_normal) < 0.0f;
    normal = front_face ? outward_normal : -outward_normal;
}

bool hit_s(in Sphere sphere, in ray r, in float t_min, in float t_max, out hit_record rec) {
    bool hit = false;
    if (near_zero(sphere.radius)) {
        hit = false;
    } else {
        const float3 oc = r.position - sphere.center;
        const float a_len = length(r.direction);
        const float a = a_len * a_len;
        const float half_b = dot(oc, r.direction);
        const float oc_len = length(oc);
        const float oc_lensq = oc_len * oc_len;
        const float c = oc_lensq - sphere.radius * sphere.radius;
        const float discriminant = half_b * half_b - a * c;
        if (discriminant < 0.0f) {
            hit = false;
        } else {
            const float sqrtd = sqrt(discriminant);
            float root = (-half_b - sqrtd) / a;
            if (root < t_min || t_max < root) {
                root = (-half_b + sqrtd) / a;
                if (root < t_min || t_max < root) {
                    rec.hit = false;
                    hit = false;
                } else {
                    rec.hit = true;
                    rec.t = root;
                    rec.p = r.position + r.direction * rec.t;
                    float3 outward_normal = (rec.p - sphere.center) / sphere.radius;
                    set_face_normal(r, outward_normal, rec.front_face, rec.normal);
                    rec.material = sphere.material;
                    hit = true;
                }
            } else {
                rec.hit = true;
                rec.t = root;
                rec.p = r.position + r.direction * rec.t;
                float3 outward_normal = (rec.p - sphere.center) / sphere.radius;
                set_face_normal(r, outward_normal, rec.front_face, rec.normal);
                rec.material = sphere.material;
                hit = true;
            }
        }
    }
    return hit;
}

bool hit_r(in ray r, in float t_min, in float t_max, out hit_record rec) {
    hit_record temp_rec;
    temp_rec.hit = false;
    temp_rec.front_face = false;
    temp_rec.t = t_max;
    temp_rec.p = float3(0.0f, 0.0f, 0.0f);
    temp_rec.normal = float3(0.0f, 0.0f, 0.0f);
    bool hit_anything = false;
    float closest = t_max;
    for (int i = 0; i < MAX_SPHERES; ++i) {
        if (hit_s(spheres[i], r, t_min, closest, temp_rec)) {
            hit_anything = true;
            closest = temp_rec.t;
            rec = temp_rec;
        }
    }
    return hit_anything;
}

#define MAT_TYPE_LAMBERTIAN 0
#define MAT_TYPE_METAL 1
#define MAT_TYPE_GLASS 2

bool scatter(in Material mat, in float2 uv, in ray ray_in, in hit_record rec, out ray ray_out) {
    switch (mat.type) {
    case MAT_TYPE_LAMBERTIAN: {
        float3 direction = rec.normal + gMaterialRoughness * normalize(float3(nrand(uv), nrand(uv), nrand(uv)));
        if (near_zero(direction)) {
            direction = rec.normal;
        }
        ray_out.position = rec.p;
        ray_out.direction = direction;
        return true;
    }
    case MAT_TYPE_METAL: {
        float3 direction = mat.metallic * reflect(normalize(ray_in.direction), rec.normal);
        ray_out.position = rec.p;
        ray_out.direction = direction + gMaterialRoughness * normalize(float3(nrand(uv), nrand(uv), nrand(uv)));
        return dot(ray_out.direction, rec.normal) > 0.0f;
    }
    case MAT_TYPE_GLASS: {
        float3 direction;
        const float refraction_ratio = rec.front_face ? 1.0f / mat.refractionIndex : mat.refractionIndex;
        const float3 unit_dir = normalize(ray_in.direction);
        const float cos_theta = min(dot(-unit_dir, rec.normal), 1.0f);
        const float sin_theta = sqrt(1.0f - cos_theta * cos_theta);
        const bool cannot_refract = refraction_ratio * sin_theta > 1.0f;
        float r0 = (1.0f - refraction_ratio) / (1.0f + refraction_ratio);
        r0 = r0 * r0;
        const float reflectance = r0 + (1.0f - r0) * pow((1.0f - cos_theta), 5.0f);
        float4 ralbedo = tRandomUV.SampleLevel(sSampler, uv.x, 0);
        float r = ralbedo.r;
        float g = ralbedo.g;
        if (cannot_refract || reflectance > nrand(float2(r, g))) {
            direction = reflect(unit_dir, rec.normal);
        }
        else {
            direction = refract(unit_dir, rec.normal, refraction_ratio);
        }
        ray_out.position = rec.p;
        ray_out.direction = direction;
        return true;
    }
    default:
        return false;
    }
}

ray get_ray(float s, float t) {
    const float theta = gVFovRadians;
    const float h = tan(theta * 0.5f);
    const float viewport_height = 2.0f * h;
    const float viewport_width = gAspectRatio * viewport_height;

    const float3 u = float3(gViewMatrix[0].x, gViewMatrix[0].y, gViewMatrix[0].z);
    const float3 v = float3(gViewMatrix[1].x, gViewMatrix[1].y, gViewMatrix[1].z);
    const float3 w = float3(gViewMatrix[2].x, gViewMatrix[2].y, gViewMatrix[2].z);

    const float3 horizontal = gFocusDistance * viewport_width * u;
    const float3 vertical = gFocusDistance * viewport_height * v;

    const float4 random_disk = gLensRadius * ((tRandomDiskResult.SampleLevel(sSampler, float2(s, t), 0) * 2.0f) - 1.0f);
    const float3 offset = u * random_disk.x + v * random_disk.y;
    const float3 lower_left_corner = gEyePosition - horizontal * 0.5f - vertical * 0.5f - gFocusDistance * w;

    ray r;
    r.position = gEyePosition + offset;
    r.direction = lower_left_corner + s * horizontal + t * vertical - gEyePosition - offset;

    return r;
}

uniform float pi = 3.141592653589793f;

float degrees_to_radians(float degrees) {
    return degrees * pi / 180.0f;
}

#define FLT_MAX 3.402823466e+38F

float3 ray_color(ray r, float2 uv, float depth) {

    hit_record rec;
    rec.p = float3(0.0f, 0.0f, 0.0f);
    rec.normal = float3(0.0f, 0.0f, 1.0f);
    rec.material;
    rec.t = 0.0f;
    rec.hit = false;
    rec.front_face = false;

    for (; depth > 0.0f; --depth) {
        if (depth <= 0.0f) {
            return float3(0.0f, 0.0f, 0.0f);
        }

        //FLT_MAX  is infinity
        if (hit_r(r, 0.001f, FLT_MAX, rec)) {
            ray scattered = r;
            if (scatter(rec.material, uv, r, rec, scattered)) {
                float3 color = float3(0.0f, 0.0f, 0.0f);
                float new_depth = depth - 1.0f;
                if (new_depth <= 0.0f) {
                    return float3(0.0f, 0.0f, 0.0f);
                }
                color += rec.material.color.rgb * color;
                return color;
            }
            return float3(0.0f, 0.0f, 0.0f);
        }

        float3 direction = normalize(r.direction);
        float t = 0.5f * (direction.y + 1.0f);
        return (1.0f - t) * float3(1.0f, 1.0f, 1.0f) + t * float3(0.5f, 0.7f, 1.0f);
    }
    return float3(0.0f, 0.0f, 0.0f);
}

float4 main(ps_input input) : SV_TARGET{
    const float pixel_x = 1.0f / gScreenWidth;
    const float pixel_y = 1.0f / gScreenHeight;
    float4 pixel_color = float4(0.0f, 0.0f, 0.0f, 1.0f);
    for (float y = gScreenHeight - pixel_x; y >= 0.0f; y -= pixel_y) {
        for (float x = 0; x < gScreenWidth; x += pixel_x) {
            for (float current_sample0 = 0; current_sample0 < gSamplesPerPixel / 2; ++current_sample0) {
                const float u = (x + tRandomUV.SampleLevel(sSampler, current_sample0 / gSamplesPerPixel, 0).r) / (gScreenWidth - pixel_x);
                const float v = (y + tRandomUV.SampleLevel(sSampler, current_sample0 / gSamplesPerPixel, 0).g) / (gScreenHeight - pixel_y);
                const ray r = get_ray(u, v);
                pixel_color += float4(ray_color(r, float2(u, v), gMaxDepth), 1.0f);
            }
            for (float current_sample1 = gSamplesPerPixel / 2; current_sample1 < gSamplesPerPixel; ++current_sample1) {
                const float u = (x + tRandomUV.SampleLevel(sSampler, current_sample1 / gSamplesPerPixel, 0).b) / (gScreenWidth - pixel_x);
                const float v = (y + tRandomUV.SampleLevel(sSampler, current_sample1 / gSamplesPerPixel, 0).a) / (gScreenHeight - pixel_y);
                const ray r = get_ray(u, v);
                pixel_color += float4(ray_color(r, float2(u, v), gMaxDepth), 1.0f);
            }
        }
    }
    return pixel_color;
}
