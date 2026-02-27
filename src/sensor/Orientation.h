#pragma once
#include <Arduino.h>
#include "SharedState.h"

// ═══════════════════════════════════════════════════════════════
//  Orientation  —  쿼터니언 처리, 센터링, 채널 변환
// ═══════════════════════════════════════════════════════════════

class Orientation {
public:
    // ── 센터 리셋 ─────────────────────────────────────────────
    // 현재 쿼터니언을 "영점"으로 기록한다
    static void resetCenter(SharedState& s) {
        if (!s.lock()) return;
        s.centerQw = s.orientation.qw;
        s.centerQx = s.orientation.qx;
        s.centerQy = s.orientation.qy;
        s.centerQz = s.orientation.qz;
        s.unlock();
        Serial.println("[ORI] Center reset");
    }

    // ── 메인 업데이트 ─────────────────────────────────────────
    // BNO085에서 새 쿼터니언이 들어올 때마다 호출
    // lock 없이 호출하되, 결과를 lock 후 SharedState에 기록
    static void update(SharedState& s, float qw, float qx, float qy, float qz) {
        // 1. 센터 기준 상대 쿼터니언 계산
        float rw, rx, ry, rz;
        relativeQuat(s.centerQw, s.centerQx, s.centerQy, s.centerQz,
                     qw, qx, qy, qz,
                     rw, rx, ry, rz);

        // 2. 오일러각 변환
        float pan, tilt, roll;
        quatToEuler(rw, rx, ry, rz, pan, tilt, roll);

        // 3. 설정 읽기 (스냅샷)
        float maxPan, maxTilt, maxRoll;
        bool invPan, invTilt, invRoll;
        {
            if (!s.lock()) return;
            maxPan  = s.config.maxPan;
            maxTilt = s.config.maxTilt;
            maxRoll = s.config.maxRoll;
            invPan  = s.config.invPan;
            invTilt = s.config.invTilt;
            invRoll = s.config.invRoll;
            s.unlock();
        }

        // 4. 인버트 적용
        if (invPan)  pan  = -pan;
        if (invTilt) tilt = -tilt;
        if (invRoll) roll = -roll;

        // 5. 채널 변환
        uint16_t chPan  = angleToCrsf(pan,  maxPan);
        uint16_t chTilt = angleToCrsf(tilt, maxTilt);
        uint16_t chRoll = angleToCrsf(roll, maxRoll);

        // 6. 결과 저장
        if (!s.lock()) return;
        s.orientation.qw = qw;
        s.orientation.qx = qx;
        s.orientation.qy = qy;
        s.orientation.qz = qz;
        s.orientation.pan  = pan;
        s.orientation.tilt = tilt;
        s.orientation.roll = roll;
        s.orientation.timestamp_us = esp_timer_get_time();
        s.channels.pan()  = chPan;
        s.channels.tilt() = chTilt;
        s.channels.roll() = chRoll;
        s.unlock();
    }

private:
    // ── q_center^-1 * q_current (상대 회전) ──────────────────
    // 센터 리셋 시점을 영점으로 하는 상대 쿼터니언 계산
    // q_rel = conj(q_c) * q_curr
    static void relativeQuat(float cw, float cx, float cy, float cz,
                              float qw, float qx, float qy, float qz,
                              float& rw, float& rx, float& ry, float& rz) {
        // conj(q_c) = (cw, -cx, -cy, -cz)
        // q_rel = conj * q_curr
        rw =  cw*qw + cx*qx + cy*qy + cz*qz;
        rx =  cw*qx - cx*qw - cy*qz + cz*qy;
        ry =  cw*qy + cx*qz - cy*qw - cz*qx;
        rz =  cw*qz - cx*qy + cy*qx - cz*qw;
    }

    // ── 쿼터니언 → ZYX 오일러각 (도) ─────────────────────────
    // 항공 관례: Yaw(pan), Pitch(tilt), Roll(roll)
    // BNO085는 NED 좌표계: X=앞, Y=오른쪽, Z=아래
    static void quatToEuler(float qw, float qx, float qy, float qz,
                            float& pan, float& tilt, float& roll) {
        // Yaw (Z축 회전, 좌우 Pan)
        float siny_cosp = 2.0f * (qw * qz + qx * qy);
        float cosy_cosp = 1.0f - 2.0f * (qy * qy + qz * qz);
        pan = atan2f(siny_cosp, cosy_cosp) * RAD_TO_DEG;

        // Pitch (Y축 회전, 상하 Tilt)
        float sinp = 2.0f * (qw * qy - qz * qx);
        sinp = constrain(sinp, -1.0f, 1.0f);   // gimbal lock 방지
        tilt = asinf(sinp) * RAD_TO_DEG;

        // Roll (X축 회전)
        float sinr_cosp = 2.0f * (qw * qx + qy * qz);
        float cosr_cosp = 1.0f - 2.0f * (qx * qx + qy * qy);
        roll = atan2f(sinr_cosp, cosr_cosp) * RAD_TO_DEG;
    }

    // ── 각도 → CRSF 채널값 ───────────────────────────────────
    // angle: -maxAngle~+maxAngle (도)
    // 출력: CRSF_MIN(172) ~ CRSF_MID(992) ~ CRSF_MAX(1811)
    static uint16_t angleToCrsf(float angle, float maxAngle) {
        if (maxAngle <= 0.0f) return CRSF_MID_US;
        float normalized = angle / maxAngle;               // -1.0 ~ +1.0
        normalized = constrain(normalized, -1.0f, 1.0f);
        int32_t raw = (int32_t)(CRSF_MID_US + normalized * CRSF_RANGE);
        return (uint16_t)constrain(raw, CRSF_MIN_US, CRSF_MAX_US);
    }
};
