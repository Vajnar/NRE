/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NRE (NOVA runtime environment).
 *
 * NRE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NRE is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#include <kobj/LocalThread.h>
#include <kobj/GlobalThread.h>
#include <kobj/Sc.h>
#include <kobj/Sm.h>
#include <kobj/Gsi.h>
#include <ipc/Connection.h>
#include <ipc/ClientSession.h>
#include <ipc/Consumer.h>
#include <stream/OStringStream.h>
#include <stream/ConsoleStream.h>
#include <services/Console.h>
#include <services/Keyboard.h>
#include <services/Mouse.h>
#include <services/ACPI.h>
#include <services/Timer.h>
#include <services/Storage.h>
#include <util/Date.h>
#include <Exception.h>

using namespace nre;

static Connection *con;

#if 0
static void read(void *) {
	while(1) {
		Session conssess(*con);
		DataSpace ds(100,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW);
		ds.share(conssess);
		Consumer<int> c(&ds);
		for(int x = 0; x < 100; ++x) {
			int *i = c.get();
			Serial::get().writef("[%p] Got %d\n",ds.virt(),*i);
			c.next();
		}
	}
}

static void write(void *) {
	Session conssess(*con);
	Pt &pt = conssess.pt(CPU::current().id);
	DataSpace ds(100,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW);
	ds.share(conssess);
	int *data = reinterpret_cast<int*>(ds.virt());
	for(uint i = 0; i < 100; ++i) {
		UtcbFrame uf;
		*data = i;
		//uf << i;
		pt.call(uf);
	}
}

struct Info {
	ConsoleSession *conssess;
	int no;
};

static void reader(void*) {
	Info *info = Thread::current()->get_tls<Info*>(Thread::TLS_PARAM);
	while(1) {
		Console::ReceivePacket *pk = info->conssess->consumer().get();
		Serial::get().writef("%u: Got c=%#x kc=%#x, flags=%#x\n",info->no,pk->character,pk->keycode,pk->flags);
		info->conssess->consumer().next();
	}
}

static void writer(void*) {
	Info *info = Thread::current()->get_tls<Info*>(Thread::TLS_PARAM);
	while(1) {
		for(int y = 0; y < 25; y++) {
			for(int x = 0; x < 80; x++) {
				Console::SendPacket pk;
				pk.x = x;
				pk.y = y;
				pk.character = 'A' + info->no;
				pk.color = x % 8;
				info->conssess->producer().produce(pk);
			}
		}
	}
}
#endif

static Connection *conscon;
static ConsoleSession *conssess;
static Connection *timercon;
static TimerSession *timer;
static UserSm sm;

static void view0(void*) {
	uint page = Thread::current()->get_tls<word_t>(Thread::TLS_PARAM);
	ConsoleStream view(*conssess,page % Console::TEXT_PAGES);
	int i = 0;
	while(1) {
		//char c = view.read();
		view << "Huhu, from page " << view.page() << ": " << i << "\n";
		i++;
	}
}

static void tick_thread(void*) {
	timevalue_t uptime,unixts;
	int i = 0;
	while(1) {
		timer->wait_for(Hip::get().freq_tsc * 1000);
		timer->get_time(uptime,unixts);
		ScopedLock<UserSm> guard(&sm);
		Serial::get() << "CPU" << CPU::current().log_id() << ": ";
		Serial::get() << ++i << " ticks, uptime=" << uptime << ", unixtime=" << unixts << ", ";
		DateInfo date;
		Date::gmtime(unixts / Timer::WALLCLOCK_FREQ,&date);
		Serial::get().writef("date: %02d.%02d.%04d %d:%02d:%02d\n",
				date.mday,date.mon,date.year,date.hour,date.min,date.sec);
	}
}

static void wait_for(StorageSession &sess,Storage::tag_type tag) {
	Storage::Packet *pk = sess.consumer().get();
	Serial::get() << "Command " << Format<Storage::tag_type>("%#x",tag) << " complete\n";
	assert(pk->tag == tag);
	assert(pk->status == 0);
	sess.consumer().next();
}

int main() {
	/*conscon = new Connection("console");
	conssess = new ConsoleSession(*conscon,CPU::count());
	for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it) {
		GlobalThread *gt = new GlobalThread(view0,it->log_id());
		gt->set_tls<uint>(Thread::TLS_PARAM,it->log_id());
		Sc *sc = new Sc(gt,Qpd());
		sc->start(String("test-thread"));
		//Sc *sc = new Sc(gt,Qpd());
		//sc->start();
	}*/

	DataSpace ds(0x2000,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW);
	Connection storagecon("storage");

	// use AHCI controller
	{
		StorageSession storage(storagecon,ds,0);

		Storage::Parameter params = storage.get_params();
		Serial::get().writef("Talking with device %s (%Lu sectors with %zu bytes each, max %zu requests)\n",
				params.name,params.sectors,params.sector_size,params.max_requests);

		// read first sector
		storage.read(1,0);
		wait_for(storage,1);
		Serial::get().writef("----\n%.512s----\n",ds.virt());

		// write first sector
		memcpy(reinterpret_cast<char*>(ds.virt()),"Foobar!!",9);
		storage.write(2,0);
		wait_for(storage,2);

		storage.flush(3);
		wait_for(storage,3);
	}

	// use IDE controller
	{
		StorageSession storage(storagecon,ds,1);

		Storage::Parameter params = storage.get_params();
		Serial::get().writef("Talking with device %s (%Lu sectors with %zu bytes each, max %zu requests)\n",
				params.name,params.sectors,params.sector_size,params.max_requests);

		// read first sector
		storage.read(4,0);
		wait_for(storage,4);
		Serial::get().writef("----\n%.512s----\n",ds.virt());

		// write second sector
		memset(reinterpret_cast<void*>(ds.virt()),0,0x200);
		memcpy(reinterpret_cast<char*>(ds.virt()),"That is a test!!",18);
		storage.write(5,1);
		wait_for(storage,5);

		storage.flush(6);
		wait_for(storage,6);
	}

	/*
	{
		timercon = new Connection("timer");
		timer = new TimerSession(*timercon);
		for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it) {
			Sc *sc = new Sc(new GlobalThread(tick_thread,it->log_id()),Qpd());
			sc->start();
		}
	}

	for(int i = 0; i < 2; ++i) {
		Connection *con = new Connection("console");
		Info *info = new Info();
		info->sess = new ConsoleSession(*con);
		info->no = i;
		Sc *sc1 = new Sc(new GlobalThread(reader,CPU::current().id),Qpd());
		sc1->ec()->set_tls(Thread::TLS_PARAM,info);
		sc1->start();
		Sc *sc2 = new Sc(new GlobalThread(writer,CPU::current().id),Qpd());
		sc2->ec()->set_tls(Thread::TLS_PARAM,info);
		sc2->start();
	}

	{
		Connection con("keyboard");
		KeyboardSession kb(con);
		Keyboard::keycode_t kc = 0;
		while(kc != Keyboard::VK_ESC) {
			Keyboard::Packet *data = kb.consumer().get();
			Serial::get().writef("Got sc=%#x kc=%#x, flags=%#x\n",data->scancode,data->keycode,data->flags);
			kc = data->keycode;
			kb.consumer().next();
		}
	}

	{
		Connection con("mouse");
		MouseSession ms(con);
		while(1) {
			Mouse::Packet *data = ms.consumer().get();
			Serial::get().writef("Got status=%#x (%u,%u,%u)\n",data->status,data->x,data->y,data->z);
			ms.consumer().next();
		}
	}
	*/

	/*
	{
		Gsi kb(1);
		for(int i = 0; i < 10; ++i) {
			kb.down();
			Serial::get().writef("Got keyboard interrupt\n");
		}
	}

	con = new Connection("screen");

	for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it) {
		//new Sc(new GlobalThread(write,it->id()),Qpd());
		Sc *sc = new Sc(new GlobalThread(read,it->id),Qpd());
		sc->start();
	}*/

	Sm sm(0);
	sm.down();
	return 0;
}
