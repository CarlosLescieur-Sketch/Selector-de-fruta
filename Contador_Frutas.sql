create database contador;

use contador;

create table frutaaceptada(
	id_acept int auto_increment primary key not null,
    numero int not null,
    fecha_subida datetime not null
);

create table frutarechazada(
	id_rech int auto_increment primary key not null,
    numero int not null,
    fecha_subida datetime not null
);