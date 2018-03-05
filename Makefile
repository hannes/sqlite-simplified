CFLAGS=-g -O0 -Wall -DHAVE_READLINE -DHAVE_EDITLINE -DHACK
LDFLAGS=-g
SYSTEM_LIBS=-ldl -lpthread -lreadline
LIBOBJS0 = alter.lo analyze.lo attach.lo auth.lo \
         backup.lo bitvec.lo btmutex.lo btree.lo build.lo \
         callback.lo complete.lo ctime.lo \
         date.lo dbpage.lo dbstat.lo delete.lo \
         expr.lo fault.lo fkey.lo \
         func.lo global.lo hash.lo \
         insert.lo legacy.lo loadext.lo \
         main.lo malloc.lo mem0.lo mem1.lo mem2.lo mem3.lo mem5.lo \
         memjournal.lo \
         mutex.lo mutex_noop.lo mutex_unix.lo mutex_w32.lo \
         notify.lo opcodes.lo os.lo os_unix.lo os_win.lo \
         pager.lo parse.lo pcache.lo pcache1.lo pragma.lo prepare.lo printf.lo \
         random.lo resolve.lo rowset.lo \
          select.lo status.lo \
         table.lo threads.lo tokenize.lo treeview.lo trigger.lo \
         update.lo util.lo vacuum.lo \
         vdbe.lo vdbeapi.lo vdbeaux.lo vdbeblob.lo vdbemem.lo vdbesort.lo \
         vdbetrace.lo wal.lo walker.lo where.lo wherecode.lo whereexpr.lo \
         utf.lo vtab.lo shell.lo

all: sqlite

clean:
	rm -f sqlite *.lo

%.lo : %.c
	$(CC) -c $(CFLAGS) $< -o $@

sqlite: $(LIBOBJS0)
	$(CC) $(LDFLAGS) $(LIBOBJS0) $(SYSTEM_LIBS) -o sqlite
