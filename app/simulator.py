from __future__ import annotations

import math
import random
from datetime import datetime, timezone


SENSORS = ("PROA_BB", "PROA_EB", "POPA_BB", "POPA_EB")
VALID_SCENARIOS = {"normal", "band", "overload", "sensor_failure"}


class VesselSimulator:
    """Gera telemetria semelhante à futura leitura do ADS1115 e MPU6050."""

    def __init__(self) -> None:
        self.scenario = "normal"
        self._phase = 0.0

    def set_scenario(self, scenario: str) -> None:
        if scenario not in VALID_SCENARIOS:
            raise ValueError(f"Cenário inválido: {scenario}")
        self.scenario = scenario

    def _base_drafts(self) -> list[float]:
        scenarios = {
            "normal": [1.42, 1.38, 1.51, 1.48],
            "band": [2.02, 1.14, 2.08, 1.18],
            "overload": [4.35, 4.32, 4.48, 4.44],
            "sensor_failure": [0.00, 1.40, 1.52, 1.47],
        }
        return scenarios[self.scenario]

    def read(self) -> dict:
        self._phase += 0.18
        base = self._base_drafts()
        drafts: list[float] = []

        for index, value in enumerate(base):
            if self.scenario == "sensor_failure" and index == 0:
                drafts.append(0.0)
                continue

            wave = math.sin(self._phase + index * 0.7) * 0.025
            noise = random.uniform(-0.008, 0.008)
            drafts.append(max(0.0, value + wave + noise))

        active = [value for value in drafts if value > 0.05]
        average = sum(active) / len(active) if active else 0.0

        port_average = (drafts[0] + drafts[2]) / 2
        starboard_average = (drafts[1] + drafts[3]) / 2
        bow_average = (drafts[0] + drafts[1]) / 2
        stern_average = (drafts[2] + drafts[3]) / 2

        band_difference = abs(port_average - starboard_average)
        trim_difference = stern_average - bow_average

        failed = [SENSORS[i] for i, value in enumerate(drafts) if value <= 0.05]
        if failed:
            alert = "FALHA_SENSOR"
        elif band_difference > 0.50:
            alert = "PERIGO_BANDA"
        elif average > 4.20:
            alert = "SOBRECARGA"
        else:
            alert = "NORMAL"

        roll = (port_average - starboard_average) * 8.0
        roll += math.sin(self._phase) * 0.25
        pitch = trim_difference * 4.0 + math.cos(self._phase) * 0.15

        if alert in {"PERIGO_BANDA", "SOBRECARGA", "FALHA_SENSOR"}:
            stability = "CRITICO"
        elif abs(roll) > 5 or abs(pitch) > 5:
            stability = "ATENCAO"
        else:
            stability = "BOA"

        sensor_data = {}
        for name, draft in zip(SENSORS, drafts):
            sensor_data[name] = {
                "draft_m": round(draft, 3),
                "pressure_kpa": round(draft * 9.80665, 2),
                "active": draft > 0.05,
            }

        gravity = 1.0
        roll_rad = math.radians(roll)
        pitch_rad = math.radians(pitch)

        return {
            "timestamp": datetime.now(timezone.utc).isoformat(),
            "mode": "SIMULATION",
            "scenario": self.scenario,
            "sensors": sensor_data,
            "imu": {
                "ax": round(math.sin(pitch_rad) * gravity, 3),
                "ay": round(math.sin(roll_rad) * gravity, 3),
                "az": round(math.cos(roll_rad) * math.cos(pitch_rad), 3),
                "gx": round(math.sin(self._phase) * 0.4, 3),
                "gy": round(math.cos(self._phase) * 0.3, 3),
                "gz": 0.0,
                "roll": round(roll, 2),
                "pitch": round(pitch, 2),
                "temperature_c": round(32.0 + math.sin(self._phase / 3) * 0.6, 2),
            },
            "metrics": {
                "average_draft_m": round(average, 3),
                "band_difference_m": round(band_difference, 3),
                "trim_difference_m": round(trim_difference, 3),
                "active_sensors": len(active),
                "failed_sensors": failed,
                "alert": alert,
                "stability": stability,
            },
        }
