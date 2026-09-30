CREATE TABLE IF NOT EXISTS settings (
    key TEXT PRIMARY KEY,
    value TEXT NOT NULL DEFAULT ''
);

CREATE TABLE IF NOT EXISTS api_keys (
    provider TEXT PRIMARY KEY,
    api_key TEXT NOT NULL DEFAULT ''
);

CREATE TABLE IF NOT EXISTS binaries (
    id TEXT PRIMARY KEY,
    sha256 TEXT,
    path TEXT,
    name TEXT,
    version TEXT,
    project_path TEXT,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL,
    last_seen_at TEXT
);

CREATE TABLE IF NOT EXISTS conversations (
    id TEXT PRIMARY KEY,
    binary_id TEXT,
    title TEXT,
    summary TEXT,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS messages (
    id TEXT PRIMARY KEY,
    conversation_id TEXT NOT NULL,
    role TEXT NOT NULL,
    content TEXT,
    tool_call_id TEXT,
    tool_name TEXT,
    tool_arguments TEXT,
    tool_calls TEXT,
    created_at TEXT NOT NULL,
    FOREIGN KEY (conversation_id) REFERENCES conversations(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS analysis_notes (
    id TEXT PRIMARY KEY,
    binary_id TEXT,
    address INTEGER,
    category TEXT,
    title TEXT,
    content TEXT,
    confidence REAL,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_binaries_sha256 ON binaries(sha256);
CREATE INDEX IF NOT EXISTS idx_binaries_path ON binaries(path);
CREATE INDEX IF NOT EXISTS idx_conversations_binary ON conversations(binary_id);
CREATE INDEX IF NOT EXISTS idx_conversations_updated ON conversations(updated_at);
CREATE INDEX IF NOT EXISTS idx_messages_conversation ON messages(conversation_id);
CREATE INDEX IF NOT EXISTS idx_analysis_notes_binary ON analysis_notes(binary_id);
