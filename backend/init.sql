-- init.sql
CREATE TABLE IF NOT EXISTS devices (
    device_id TEXT PRIMARY KEY,
    meta JSONB,
    updated_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP
);
