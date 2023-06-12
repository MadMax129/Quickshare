create table if not exists 'Sessions' (
    'name' TEXT not null unique,
    'id' INTEGER PRIMARY KEY,
    'time' DATETIME not null default (datetime(CURRENT_TIMESTAMP, 'localtime'))
);