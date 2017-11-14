/* create nodes table */
CREATE TABLE nodes(
	id INTEGER PRIMARY KEY NOT NULL UNIQUE,
	rack_no INTEGER NOT NULL,
	chassis_no INTEGER NOT NULL,
	mac_address TEXT NOT NULL,
	enabled INTEGER DEFAULT 1,
	config TEXT NOT NULL,
	UNIQUE(rack_no, chassis_no)
);

/* insert some nodes */
INSERT into nodes(rack_no, chassis_no, mac_address, enabled, config) 
	VALUES(1, 1, "1:2:3:4", 1, "myconfig");
INSERT into nodes(rack_no, chassis_no, mac_address, enabled, config) 
	VALUES(1, 2, "5:6:7:8", 1, "myconfig");

/* create errors table */
CREATE TABLE errors(
	id INTEGER PRIMARY KEY NOT NULL UNIQUE,
	node_id INTEGER NOT NULL,
	recv_time INTEGER NOT NULL,
	description TEXT NOT NULL,
	valve_no INTEGER DEFAULT -1,
	enabled INTEGER DEFAULT 1
);

/* insert some errors */
INSERT into errors(node_id, recv_time, description)
	VALUES(1, 100, "node exploded");
INSERT into errors(node_id, recv_time, description)
	VALUES(1, 90, "node about to explode");
INSERT into errors(node_id, recv_time, description, enabled)
	VALUES(2, 10, "blah blah blah", 0);
INSERT into errors(node_id, recv_time, description, enabled, valve_no)
	VALUES(2, 80, "valve causing a chain reaction", 1, 2);


/* hmm I wonder what happened here... */
.print "\n--Error Traceback:"
SELECT nodes.rack_no, nodes.chassis_no, errors.description
	from errors 
	INNER JOIN nodes
	ON errors.node_id = nodes.id
	WHERE (errors.enabled != 0)
	ORDER BY recv_time;

/* find all the errors relating to node 2 */
.print "\n--ERRORS relating to node 2"
SELECT errors.description, errors.recv_time, errors.valve_no, errors.enabled
	from errors
	INNER JOIN nodes
	ON errors.node_id = nodes.id
	WHERE nodes.id = 2
	ORDER BY errors.recv_time;

/* disable chassis number 2 for now */
UPDATE nodes SET enabled = 0 WHERE rack_no = 1 AND chassis_no = 2;

.print "\n--NODES after update:"
SELECT * from nodes;

.print "\n--ERROR count:"
SELECT COUNT(*) FROM errors WHERE (errors.enabled != 0);

/* query for search_clickable */
.print "\n--Fancy query"
SELECT errors.description, nodes.rack_no, nodes.chassis_no, errors.valve_no
	FROM errors
	INNER JOIN nodes
	ON errors.node_id = nodes.id
	WHERE nodes.enabled = 1 AND errors.enabled = 1 AND nodes.rack_no = 1
	ORDER BY errors.recv_time;