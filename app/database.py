from __future__ import annotations

import json
import sqlite3
from pathlib import Path


class TelemetryDatabase:
    def __init__(self, database_path: Path) -> None:
        database_path.parent.mkdir(parents=True, exist_ok=True)
        self.database_path = database_path
        self._create_table()

    def _connect(self) -> sqlite3.Connection:
        connection = sqlite3.connect(self.database_path)
        connection.row_factory = sqlite3.Row
        return connection

    def _create_table(self) -> None:
        with self._connect() as connection:
            connection.execute(
                """
                CREATE TABLE IF NOT EXISTS telemetry (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    timestamp TEXT NOT NULL,
                    scenario TEXT NOT NULL,
                    average_draft_m REAL NOT NULL,
                    band_difference_m REAL NOT NULL,
                    roll REAL NOT NULL,
                    pitch REAL NOT NULL,
                    alert TEXT NOT NULL,
                    payload_json TEXT NOT NULL
                )
                """
            )

    def save(self, telemetry: dict) -> None:
        metrics = telemetry["metrics"]
        imu = telemetry["imu"]
        with self._connect() as connection:
            connection.execute(
                """
                INSERT INTO telemetry (
                    timestamp, scenario, average_draft_m, band_difference_m,
                    roll, pitch, alert, payload_json
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                """,
                (
                    telemetry["timestamp"],
                    telemetry["scenario"],
                    metrics["average_draft_m"],
                    metrics["band_difference_m"],
                    imu["roll"],
                    imu["pitch"],
                    metrics["alert"],
                    json.dumps(telemetry, ensure_ascii=False),
                ),
            )

    def history(self, limit: int = 60) -> list[dict]:
        safe_limit = max(1, min(limit, 1000))
        with self._connect() as connection:
            rows = connection.execute(
                """
                SELECT id, timestamp, scenario, average_draft_m,
                       band_difference_m, roll, pitch, alert
                FROM telemetry
                ORDER BY id DESC
                LIMIT ?
                """,
                (safe_limit,),
            ).fetchall()
        return [dict(row) for row in rows]
