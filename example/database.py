from pysqlite2 import dbapi2 as sqlite3

con = sqlite3.connect(":memory:")
cur = con.cursor()
cur.execute('create table test (id varchar(5), info varchar(20));')
cur.execute('insert into test values ("123", "hello world");')

def query(qid):
    cur.execute("select * from test where id=" + qid);  #this is the bad case.
    return cur.fetchall()
