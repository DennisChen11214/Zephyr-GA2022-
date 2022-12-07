#include "quatf.h"

#define _USE_MATH_DEFINES
#include <math.h>

quatf_t quatf_look_at(vec3f_t dir, vec3f_t up)
{
	/* From unity answers: https://answers.unity.com/questions/467614/what-is-the-source-code-of-quaternionlookrotation.html */

    // Convert to unity's axes
    vec3f_t unity_dir = (vec3f_t){ .x = -dir.x, .y = dir.z, .z = dir.y };
    vec3f_t unity_up = (vec3f_t){ .x = -up.x, .y = up.z, .z = up.y };

    vec3f_t vector = vec3f_norm(unity_dir);
    vec3f_t vector2 = vec3f_norm(vec3f_cross(unity_up, vector));
    vec3f_t vector3 = vec3f_cross(vector, vector2);
    float m00 = vector2.x;
    float m01 = vector2.y;
    float m02 = vector2.z;
    float m10 = vector3.x;
    float m11 = vector3.y;
    float m12 = vector3.z;
    float m20 = vector.x;
    float m21 = vector.y;
    float m22 = vector.z;


    float num8 = (m00 + m11) + m22;
    quatf_t unity_quat = quatf_identity();
    if (num8 > 0)
    {
        float num = (float)sqrtf(num8 + 1);
        unity_quat.w = num * 0.5f;
        num = 0.5f / num;
        unity_quat.x = (m12 - m21) * num;
        unity_quat.y = (m20 - m02) * num;
        unity_quat.z = (m01 - m10) * num;
    }
    else if ((m00 >= m11) && (m00 >= m22))
    {
        float num7 = (float)sqrtf(((1 + m00) - m11) - m22);
        float num4 = 0.5f / num7;
        unity_quat.x = 0.5f * num7;
        unity_quat.y = (m01 + m10) * num4;
        unity_quat.z = (m02 + m20) * num4;
        unity_quat.w = (m12 - m21) * num4;
    }
    else if (m11 > m22)
    {
        float num6 = (float)sqrtf(((1 + m11) - m00) - m22);
        float num3 = 0.5f / num6;
        unity_quat.x = (m10 + m01) * num3;
        unity_quat.y = 0.5f * num6;
        unity_quat.z = (m21 + m12) * num3;
        unity_quat.w = (m20 - m02) * num3;
    }
    else 
    {
        float num5 = (float)sqrtf(((1 + m22) - m00) - m11);
        float num2 = 0.5f / num5;
        unity_quat.x = (m20 + m02) * num2;
        unity_quat.y = (m21 + m12) * num2;
        unity_quat.z = 0.5f * num5;
        unity_quat.w = (m01 - m10) * num2;
    }
    //Convert back to this coordinate system
    quatf_t quat = (quatf_t){ .x = -unity_quat.x, .y = unity_quat.z, .z = unity_quat.y, unity_quat.w };
    return quat;
}

vec3f_t quatf_to_eulers(quatf_t q)
{
	/* From wikipedia: https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles */

	// roll (x-axis rotation)
	float sinr = 2.0f * (q.w * q.x + q.y * q.z);
	float cosr = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
	float roll = atan2f(sinr, cosr);

	// pitch (y-axis rotation)
	float pitch = 0;
	float sinp = 2.0f * (q.w * q.y - q.z * q.x);
	if (fabsf(sinp) >= 1)
	{
		pitch = copysignf((float)M_PI / 2.0f, sinp); // use 90 degrees if out of range
	}
	else
	{
		pitch = asinf(sinp);
	}

	// yaw (z-axis rotation)
	float siny = 2.0f * (q.w * q.z + q.x * q.y);
	float cosy = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
	float yaw = atan2f(siny, cosy);

	return (vec3f_t) { .x = roll, .x = pitch, .x = yaw };
}

quatf_t quatf_from_eulers(vec3f_t euler_angles)
{
	/* From wikipedia: https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles */

	float roll = euler_angles.x;
	float pitch = euler_angles.y;
	float yaw = euler_angles.z;

	float cy = cosf(yaw * 0.5f);
	float sy = sinf(yaw * 0.5f);
	float cr = cosf(roll * 0.5f);
	float sr = sinf(roll * 0.5f);
	float cp = cosf(pitch * 0.5f);
	float sp = sinf(pitch * 0.5f);

	return (quatf_t)
	{
		.w = cy * cr * cp + sy * sr * sp,
		.x = cy * sr * cp - sy * cr * sp,
		.y = cy * cr * sp + sy * sr * cp,
		.z = sy * cr * cp - cy * sr * sp,
	};
}
