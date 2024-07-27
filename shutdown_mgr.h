/* 
 * File:   shutdown_mgr.h
 * Author: emtszeb
 *
 * Created on July 27, 2024, 11:18 AM
 */

#ifndef SHUTDOWN_MGR_H
#define	SHUTDOWN_MGR_H

#ifdef	__cplusplus
extern "C" {
#endif

    void ShutdownMgrInit(void);
    void ShutdownButtonPressed(void);

#ifdef	__cplusplus
}
#endif

#endif	/* SHUTDOWN_MGR_H */

