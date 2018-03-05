create table test(i integer, j integer);
insert into test values (42, 1),(43, 2),(44, 3);
EXPLAIN select i from test where i > 42;

select i from test where i > 42;
