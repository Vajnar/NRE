/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <RCU.h>
#include <cstring>

#include "ConsoleSessionView.h"
#include "ConsoleSessionData.h"

using namespace nul;

ForwardCycler<CPU::iterator,LockPolicyDefault<SpinLock> > ConsoleSessionView::_cpus(CPU::begin(),CPU::end());
// 0 and 1 is for boot and HV
uint ConsoleSessionView::_next_uid = 2;

ConsoleSessionView::ConsoleSessionView(ConsoleSessionData *sess,uint id,DataSpace *in_ds,DataSpace *out_ds)
	: RCUObject(), DListItem(), _id(id), _uid(Atomic::add(&_next_uid,+1)), _pos(0),
	  _ec(receiver,_cpus.next()->id), _sc(&_ec,Qpd()), _in_ds(in_ds), _out_ds(out_ds),
	  _buffer(ExecEnv::PAGE_SIZE,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW),
	  _prod(in_ds,false,false), _cons(out_ds,false), _sess(sess) {
	_ec.set_tls<ConsoleSessionView*>(Ec::TLS_PARAM,this);
	memset(reinterpret_cast<void*>(_buffer.virt()),0,_buffer.size());
	_sc.start();
}

ConsoleSessionView::~ConsoleSessionView() {
	delete _out_ds;
	delete _in_ds;
}

void ConsoleSessionView::invalidate() {
	_cons.stop();
}

bool ConsoleSessionView::is_active() const {
	return _sess->is_active(this);
}

void ConsoleSessionView::put(const Console::SendPacket &pk) {
	if(pk.y < Screen::ROWS && pk.x < Screen::COLS) {
		uint8_t *buf = reinterpret_cast<uint8_t*>(_buffer.virt());
		uint8_t *pos = buf + (_pos + pk.y * Screen::COLS * 2 + pk.x * 2) % Screen::SIZE;
		*pos = pk.character;
		*(pos + 1) = pk.color;
		ConsoleService::get()->screen()->paint(_uid,pk.x,pk.y,pos,2);
	}
}

void ConsoleSessionView::scroll() {
	uint8_t *buf = reinterpret_cast<uint8_t*>(_buffer.virt());
	memset(buf + _pos,0,Screen::COLS * 2);
	_pos = (_pos + Screen::COLS * 2) % Screen::SIZE;
}

void ConsoleSessionView::repaint() {
	ConsoleService::get()->screen()->paint(
			_uid,0,0,reinterpret_cast<uint8_t*>(_buffer.virt() + _pos),Screen::SIZE - _pos);
	ConsoleService::get()->screen()->paint(
			_uid,0,(Screen::SIZE - _pos) / (Screen::COLS * 2),reinterpret_cast<uint8_t*>(_buffer.virt()),_pos);
}

void ConsoleSessionView::receiver(void *) {
	ScopedLock<RCULock> guard(&RCU::lock());
	ConsoleService *s = ConsoleService::get();
	ConsoleSessionView *view = Ec::current()->get_tls<ConsoleSessionView*>(Ec::TLS_PARAM);
	Consumer<Console::SendPacket> &cons = view->cons();
	for(Console::SendPacket *pk; (pk = cons.get()) != 0; cons.next()) {
		switch(pk->cmd) {
			case Console::WRITE:
				view->put(*pk);
				break;

			case Console::SCROLL:
				view->scroll();
				if(view->is_active())
					view->repaint();
				//if(view->is_active())
				//	view->repaint();
					//s->screen().scroll(view->uid());
				break;
		}
	}
}