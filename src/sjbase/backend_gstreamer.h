/*******************************************************************************
 *
 *                                 Silverjuke
 *     Copyright (C) 2016 Björn Petersen Software Design and Development
 *                   Contact: r10s@b44t.com, http://b44t.com
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see http://www.gnu.org/licenses/ .
 *
 *******************************************************************************
 *
 * File:    backend_gstreamer.h
 * Authors: Björn Petersen
 * Purpose: GSteamer Backend
 *
 ******************************************************************************/


#ifndef __SJ_BACKEND_GSTREAMER_H__
#define __SJ_BACKEND_GSTREAMER_H__


#include <sjbase/backend.h>
#include <gst/gst.h>


class SjGstreamerBackendStream;


class SjGstreamerBackend : public SjBackend
{
public:
	                 SjGstreamerBackend  (SjBackendId);
	void             GetLittleOptions    (SjArrayLittleOption&);
	SjBackendStream* CreateStream        (const wxString& url, long seekMs, SjBackendCallback*, void* userdata);
	SjBackendState   GetDeviceState      ();
	void             SetDeviceState      (SjBackendState);
	void             SetDeviceVol        (double gain);

/*private:
however, declared as public to be usable from callbacks (for speed reasons, this avoids one level of iteration)*/
	wxString         m_iniPipeline;

protected:
	void             ReleaseBackend      () { SetDeviceState(SJBE_STATE_CLOSED); delete this; }
};


class SjGstreamerBackendStream : public SjBackendStream
{
public:
    void                GetTime             (long& totalMs, long& elapsedMs); // -1=unknown
    void                SeekAbs             (long ms);

/*private:
however, declared as public to be usable from callbacks (for speed reasons, this avoids one level of iteration)*/
    SjGstreamerBackendStream(const wxString& url, SjGstreamerBackend* backend, SjBackendCallback* cb, void* userdata)
		: SjBackendStream(url, backend, cb, userdata)
    {
		m_backend      = backend;
		m_pipeline     = NULL;
		m_bus_watch_id = 0;
		m_capsChecked  = false;
		m_eosSend      = false;
    }

	GstElement*         m_pipeline;
	guint               m_bus_watch_id;
	SjGstreamerBackend* m_backend;
	bool                m_capsChecked;
	bool                m_eosSend;
	void                set_pipeline_state  (GstState s);

protected:
	void                ReleaseStream       ();
};


#endif // __SJ_BACKEND_GSTREAMER_H__
