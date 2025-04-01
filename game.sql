-- Borra y crea la base de datos
DROP DATABASE IF EXISTS game;
CREATE DATABASE game;
USE game;

-- Tablas
CREATE TABLE Game (
	GameID INTEGER PRIMARY KEY NOT NULL, 
	points INTEGER NOT NULL,
	fecha VARCHAR(50) NOT NULL,
	time VARCHAR(50),
	duration FLOAT,
	winner VARCHAR(50) NOT NULL
)ENGINE = InnoDB;

CREATE TABLE Player (
	username VARCHAR(50) PRIMARY KEY NOT NULL, 
	password VARCHAR(50) NOT NULL
)ENGINE = InnoDB;

CREATE TABLE Questions (
	questionID INTEGER PRIMARY KEY NOT NULL,
	question VARCHAR(50) NOT NULL
)ENGINE = InnoDB;

CREATE TABLE Answers (
	answerID INTEGER PRIMARY KEY NOT NULL,
	answer VARCHAR(50) NOT NULL
)ENGINE = InnoDB;

CREATE TABLE QuestionsRel (
	questID INTEGER,
	answID INTEGER NOT NULL,
	FOREIGN KEY (questID) REFERENCES Questions(questionID),
	FOREIGN KEY (answID) REFERENCES Answers(answerID)
)ENGINE = InnoDB;

CREATE TABLE Participation (
	Player VARCHAR(50) NOT NULL,
	Game INTEGER,
	FOREIGN KEY (Player) REFERENCES Player(username),
	FOREIGN KEY (Game) REFERENCES Game(GameID)
)ENGINE = InnoDB;


USE game;

-- Datos de prueba
-- Jugadores
INSERT INTO Player (username, password) VALUES ('klp', 'klp');
INSERT INTO Player (username, password) VALUES ('rival3', '1234');
INSERT INTO Player (username, password) VALUES ('rival4', '1234');

-- Partidas
INSERT INTO Game (GameID, points, fecha, time, duration, winner)
VALUES 
  (1, 120, '2024-01-01', '12:00', 10.5, 'klp'),
  (2, 180, '2024-01-02', '15:30', 8, 'klp');

-- Participaciones
INSERT INTO Participation (Player, Game) VALUES ('klp', 1);
INSERT INTO Participation (Player, Game) VALUES ('klp', 2);
INSERT INTO Participation (Player, Game) VALUES ('rival3', 1);
INSERT INTO Participation (Player, Game) VALUES ('rival4', 2);
