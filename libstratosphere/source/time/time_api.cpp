/*
 * Copyright (c) Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stratosphere.hpp>

#if defined(ATMOSPHERE_OS_HORIZON)
extern "C" {

    extern TimeServiceType __nx_time_service_type;

}
#endif

namespace ams::time {

    namespace {

        enum InitializeMode {
            InitializeMode_None,
            InitializeMode_Normal,
            InitializeMode_Menu,
            InitializeMode_System,
            InitializeMode_Repair,
            InitializeMode_SystemUser,
        };

        constinit u32 g_initialize_count = 0;
        constinit InitializeMode g_initialize_mode = InitializeMode_None;

        constinit os::SdkMutex g_initialize_mutex;

        Result InitializeImpl(InitializeMode mode) {
            std::scoped_lock lk(g_initialize_mutex);

            if (g_initialize_count > 0) {
                AMS_ABORT_UNLESS(mode == g_initialize_mode);
                g_initialize_count++;
                R_SUCCEED();
            }

            #if defined(ATMOSPHERE_OS_HORIZON)
            switch (mode) {
                case InitializeMode_Normal:     __nx_time_service_type = ::TimeServiceType_User;       break;
                case InitializeMode_Menu:       __nx_time_service_type = ::TimeServiceType_Menu;       break;
                case InitializeMode_System:     __nx_time_service_type = ::TimeServiceType_System;     break;
                case InitializeMode_Repair:     __nx_time_service_type = ::TimeServiceType_Repair;     break;
                case InitializeMode_SystemUser: __nx_time_service_type = ::TimeServiceType_SystemUser; break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }

            R_TRY(::timeInitialize());
            #else
            AMS_ABORT("TODO");
            #endif

            g_initialize_count++;
            g_initialize_mode = mode;
            R_SUCCEED();
        }

    }

    Result Initialize() {
        R_RETURN(InitializeImpl(InitializeMode_Normal));
    }

    Result InitializeForSystem() {
        R_RETURN(InitializeImpl(InitializeMode_System));
    }

    Result InitializeForSystemUser() {
        if (hos::GetVersion() >= hos::Version_9_0_0) {
            R_RETURN(InitializeImpl(InitializeMode_SystemUser));
        } else {
            R_RETURN(InitializeImpl(InitializeMode_Normal));
        }
    }

    Result Finalize() {
        std::scoped_lock lk(g_initialize_mutex);

        if (g_initialize_count > 0) {
            if ((--g_initialize_count) == 0) {
                #if defined(ATMOSPHERE_OS_HORIZON)
                ::timeExit();
                #else
                AMS_ABORT("TODO");
                #endif
                g_initialize_mode = InitializeMode_None;
            }
        }

        R_SUCCEED();
    }

    bool IsInitialized() {
        std::scoped_lock lk(g_initialize_mutex);

        return g_initialize_count > 0;
    }

    bool IsValidDate(int year, int month, int day) {
        return impl::util::IsValidDate(year, month, day);
    }

    Result GetElapsedSecondsBetween(s64 *out, const SteadyClockTimePoint &from, const SteadyClockTimePoint &to) {
        R_RETURN(impl::util::GetSpanBetween(out, from, to));
    }

}
