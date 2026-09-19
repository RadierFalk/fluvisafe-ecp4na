from __future__ import annotations

import asyncio
from contextlib import asynccontextmanager
from pathlib import Path

from fastapi import FastAPI, HTTPException, Query
from fastapi.responses import FileResponse

from app.database import TelemetryDatabase
from app.simulator import VALID_SCENARIOS, VesselSimulator


BASE_DIR = Path(__file__).resolve().parent.parent
DATABASE_PATH = BASE_DIR / "data" / "fluvisafe.db"

simulator = VesselSimulator()
database = TelemetryDatabase(DATABASE_PATH)
latest_telemetry: dict = simulator.read()


async def acquisition_loop() -> None:
    global latest_telemetry
    while True:
        latest_telemetry = simulator.read()
        database.save(latest_telemetry)
        await asyncio.sleep(1)


@asynccontextmanager
async def lifespan(_: FastAPI):
    task = asyncio.create_task(acquisition_loop())
    try:
        yield
    finally:
        task.cancel()
        try:
            await task
        except asyncio.CancelledError:
            pass


app = FastAPI(
    title="FluviSafe API",
    version="2.0.0-simulation",
    lifespan=lifespan,
)


@app.get("/", include_in_schema=False)
def dashboard() -> FileResponse:
    return FileResponse(BASE_DIR / "flivisafe.html")


@app.get("/flivisafe.css", include_in_schema=False)
def stylesheet() -> FileResponse:
    return FileResponse(BASE_DIR / "flivisafe.css", media_type="text/css")


@app.get("/flivisafe.js", include_in_schema=False)
def javascript() -> FileResponse:
    return FileResponse(BASE_DIR / "flivisafe.js", media_type="text/javascript")


@app.get("/api/health")
def health() -> dict:
    return {"status": "ok", "mode": "simulation"}


@app.get("/api/telemetry")
def telemetry() -> dict:
    return latest_telemetry


@app.get("/api/history")
def history(limit: int = Query(default=60, ge=1, le=1000)) -> dict:
    return {"items": database.history(limit), "limit": limit}


@app.post("/api/simulation/scenario/{scenario}")
def change_scenario(scenario: str) -> dict:
    if scenario not in VALID_SCENARIOS:
        raise HTTPException(
            status_code=400,
            detail={"valid_scenarios": sorted(VALID_SCENARIOS)},
        )
    simulator.set_scenario(scenario)
    return {"status": "ok", "scenario": scenario}
