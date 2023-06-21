create table if not exists 'Sessions' (
    'name' TEXT not null unique,
    'id' INTEGER PRIMARY KEY,
    'time' DATETIME not null default (datetime(CURRENT_TIMESTAMP, 'localtime'))
);

create table if not exists 'Transfers'(
    transfer_id INTEGER PRIMARY KEY,
    creator_id INTEGER not null
);

create table if not exists 'TransferClients' (
    transfer_id INTEGER not null,
    client_id INTEGER not null,
    accepted INTEGER default -1,
    FOREIGN KEY(transfer_id) REFERENCES Transfers(transfer_id)
);

-- Add a counter for statistics