create table test(i integer);
insert into test values (42),(43),(44);
EXPLAIN select * from test where i > 42;

select * from test where i > 42;
