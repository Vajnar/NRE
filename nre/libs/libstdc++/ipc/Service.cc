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

#include <ipc/Service.h>
#include <Logging.h>

namespace nre {

ServiceSession *Service::new_session() {
	ScopedLock<UserSm> guard(&_sm);
	for(size_t i = 0; i < MAX_SESSIONS; ++i) {
		if(_sessions[i] == 0) {
			LOG(Logging::SERVICES,Serial::get() << "Creating session " << i
					<< " (caps=" << _caps + (i << CPU::order()) << ")\n");
			add_session(create_session(i,_caps + (i << CPU::order()),_func));
			return _sessions[i];
		}
	}
	throw ServiceException(E_CAPACITY,"No free sessions");
}

void Service::destroy_session(capsel_t pid) {
	ScopedLock<UserSm> guard(&_sm);
	size_t i = (pid - _caps) >> CPU::order();
	LOG(Logging::SERVICES,Serial::get() << "Destroying session " << i << "\n");
	ServiceSession *sess = _sessions[i];
	if(!sess)
		throw ServiceException(E_NOT_FOUND,32,"Session %zu does not exist",i);
	remove_session(sess);
}

}