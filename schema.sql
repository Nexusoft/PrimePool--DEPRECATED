CREATE DATABASE IF NOT EXISTS nxspool;

USE nxspool;

CREATE TABLE IF NOT EXISTS pool_data
(
	id INT NOT NULL DEFAULT 0,
	round_number INT NOT NULL ,
	block_number INT DEFAULT NULL,
	round_reward INT DEFAULT NULL,
	total_shares INT DEFAULT NULL,
	connection_count INT DEFAULT NULL,
	PRIMARY KEY (id)
	
) ENGINE=INNODB;

CREATE TABLE IF NOT EXISTS round_history
(
	round_number INT NOT NULL ,
	block_number INT DEFAULT NULL,
	block_hash VARCHAR(1024) NOT NULL,
	round_reward INT DEFAULT NULL,
	total_shares INT DEFAULT NULL,
	block_finder VARCHAR(255) NOT NULL,
	orphan INT DEFAULT NULL,
	block_found_time DATETIME NOT NULL,
	PRIMARY KEY (round_number)
	
) ENGINE=INNODB;

CREATE TABLE IF NOT EXISTS connection_history
(
	account_address VARCHAR(255) NOT NULL,
    series_time DATETIME NOT NULL,
    last_save_time DATETIME NOT NULL,
    connection_count INT DEFAULT NULL,
    pps DOUBLE DEFAULT NULL,
    wps DOUBLE DEFAULT NULL,
    PRIMARY KEY (account_address, series_time)
	
) ENGINE=INNODB;

CREATE TABLE IF NOT EXISTS account_data
(
	account_address VARCHAR(255) NOT NULL,
    last_save_time DATETIME NOT NULL,
    connection_count INT DEFAULT NULL,
    round_shares INT DEFAULT NULL,
    balance INT DEFAULT NULL,
    pending_payout INT DEFAULT NULL,
    PRIMARY KEY (account_address)
	
) ENGINE=INNODB;


CREATE TABLE IF NOT EXISTS earnings_history
(
	account_address VARCHAR(255) NOT NULL,
	round_number INT NOT NULL ,
	block_number INT DEFAULT NULL,
    round_shares INT DEFAULT NULL,
    amount_earned INT DEFAULT NULL,
	datetime DATETIME NOT NULL,
    KEY (account_address),
    KEY (account_address, round_number)
	
) ENGINE=INNODB;

CREATE TABLE IF NOT EXISTS payment_history
(
	account_address VARCHAR(255) NOT NULL,
	round_number INT NOT NULL ,
	block_number INT DEFAULT NULL,
    amount_paid INT DEFAULT NULL,
	datetime DATETIME NOT NULL,
    KEY (account_address),
    KEY (account_address, block_number)
	
) ENGINE=INNODB;