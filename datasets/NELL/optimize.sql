-- Link surrogate keys.

UPDATE EntityLiteralStrings ELS
SET entityId = E.id
FROM Entities E
WHERE E.nellId = ELS.entityNellId
;

UPDATE EntityCategories EC
SET entityId = E.id
FROM Entities E
WHERE E.nellId = EC.entityNellId
;

UPDATE Triples T
SET head = E.id
FROM Entities E
WHERE E.nellId = T.headNellId
;

UPDATE Triples T
SET relation = R.id
FROM Relations R
WHERE R.nellId = T.relationNellId
;

UPDATE Triples T
SET tail = E.id
FROM Entities E
WHERE E.nellId = T.tailNellId
;

-- Drop redundant columns.
ALTER TABLE EntityLiteralStrings
DROP COLUMN entityNellId
;

ALTER TABLE EntityCategories
DROP COLUMN entityNellId
;

ALTER TABLE Triples
DROP COLUMN headNellId,
DROP COLUMN relationNellId,
DROP COLUMN tailNellId
;
