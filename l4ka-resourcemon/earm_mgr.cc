/*****************************************************************
 * Source file : accounting_idl.idl
 * Platform    : V4 IA32
 * Mapping     : CORBA C
 * 
 * Generated by IDL4 1.0.2 (roadrunner) on 25/01/2006 16:28
 * Report bugs to haeberlen@ira.uka.de
 *****************************************************************/
#include "earm.h"
#include "earm_idl_client.h"

hthread_t *earmmanager_thread;

resource_t resources[UUID_IEarm_ResMax];

#define LOGFILE_SIZE (1 << PAGE_BITS)
typedef L4_Word8_t res_logfile_t[LOGFILE_SIZE] __attribute__ ((aligned (LOGFILE_SIZE)));
res_logfile_t res_logfile[UUID_IEarm_ResMax];

L4_Word_t max_uuid_cpu;

/* Interface Iounting::Manager */

IDL4_INLINE void IEarm_Manager_register_resource_implementation(CORBA_Object _caller, const guid_t guid, idl4_fpage_t *fp, idl4_server_environment *_env)
{
    /* implementation of Iounting::Manager::register_resource */

   
    // special handling for cpu (don't map, since we are in the same address space)
    if ( guid >= UUID_IEarm_ResCPU_Min && guid <= UUID_IEarm_ResCPU_Max ) {
	resources[guid].shared = (IEarm_shared_t *) &res_logfile[guid];
	resources[guid].tid = _caller;
	return;
    }

    if( guid < UUID_IEarm_ResMax ){
	L4_Word_t addr = 0;
	L4_Word_t haddr = (L4_Word_t) &res_logfile[guid];
	L4_Word_t privileges = 7;

	idl4_fpage_set_base( fp, addr );
	idl4_fpage_set_page( fp, L4_FpageLog2( haddr, PAGE_BITS) );
	idl4_fpage_set_mode( fp, IDL4_MODE_MAP );
	idl4_fpage_set_permissions( fp, privileges );

	resources[guid].shared = (IEarm_shared_t *) haddr;
	resources[guid].tid = _caller;
    } else
	CORBA_exception_set( _env, ex_IEarm_Manager_invalid_guid_format, NULL );
    return;
}

IDL4_PUBLISH_IEARM_MANAGER_REGISTER_RESOURCE(IEarm_Manager_register_resource_implementation);

IDL4_INLINE void IEarm_Manager_budget_resource_implementation(CORBA_Object _caller, const guid_t guid, L4_Word_t logid, L4_Word_t budget, idl4_server_environment *_env)
{
    
    if (logid >= L4_LOG_MAX_LOGIDS || (guid >= UUID_IEarm_ResMax && guid != 100))
    {
	printf("EARM: invalid  request guid %d logid %d budget %d\n", guid, logid, budget);
	    
	CORBA_exception_set( _env, ex_IEarm_Manager_invalid_guid_format, NULL );
	return;
    }

    switch (guid)
    {
    case UUID_IEarm_ResDisk:
	printf("EARM: set disk budget guid %d logid %d budget %d\n", 
	       guid, logid, budget);
	eas_disk_budget[logid] = budget;
	break;
    case UUID_IEarm_ResCPU_Min ... UUID_IEarm_ResCPU_Max:
	printf("EARM: set CPU budget guid %d logid %d budget %d\n", guid, logid, budget);
	if (l4_pmsched_enabled)
	{
	    virq_t *virq = get_virq();
	    if (logid == 0)
		virq->cpower = budget * 100;
	    virq->vctx[logid].ticket = budget;
	}
	eas_cpu_budget[guid][logid] = budget;
	    
	break;
    case 100:
	if (l4_hsched_enabled)
	{
	    printf("EARM: set CPU stride guid %d logid %d budget %d\n", 
		   guid, logid, budget);
		
	    L4_Word_t stride, result, sched_control;
	    vm_t * vm = get_vm_allocator()->space_id_to_vm( logid - VM_LOGID_OFFSET );
	    
	    //Restride logid
	    stride = budget;
	    sched_control = 16;
                        
	    result = L4_HS_Schedule(vm->get_first_tid(), sched_control, vm->get_first_tid(), 0, stride, &sched_control);
                        
	    if (!result)
	    {
		printf("Error: failure setting scheduling stride for VM TID %t result %d, errcode %s",
		       vm->get_first_tid(), result, L4_ErrorCode_String(L4_ErrorCode()));
		L4_KDB_Enter("EARM scheduling error");
	    }	
	}
	else if (l4_pmsched_enabled)
	{
	    if (logid < 3)
	    {
		
		printf("EARM: set EAS scheduler to %C\n", DEBUG_TO_4CHAR(virq_scheduler_string[logid]));
		get_virq()->scheduler = logid;
	    }
	    
	}
	break;
    default:
	printf("EARM: unused guid %d logid %d budget %d\n", 
	       guid, logid, budget);
	break;
    }
	    
	
}
IDL4_PUBLISH_IEARM_MANAGER_BUDGET_RESOURCE(IEarm_Manager_budget_resource_implementation);


IDL4_INLINE void  IEarm_Manager_resource_request_implementation(CORBA_Object  _caller, 
								const guid_t  guid, 
								const L4_ThreadId_t *client,
								idl4_server_environment * _env)
{
    //printf("EARM: resource request %t resource %d client %d\n", _caller, guid, client_space);

}
IDL4_PUBLISH_IEARM_MANAGER_RESOURCE_REQUEST(IEarm_Manager_resource_request_implementation);



IDL4_INLINE void IEarm_Manager_open_implementation(CORBA_Object _caller, const guid_t guid, L4_ThreadId_t *tid, idl4_server_environment *_env)
{
    /* implementation of Iounting::Manager::open */

    L4_Word_t logid = tid_space_t::tid_to_space_id(_caller) + VM_LOGID_OFFSET;

    printf("EARM: open logid %d thread %t opens guid %d\n", logid, _caller, guid);
    //L4_KDB_Enter("open");

    if( (guid < UUID_IEarm_ResMax) && (!L4_IsNilThread(resources[guid].tid)) ) {
      resources[guid].shared->clients[logid].limit = EARM_UNLIMITED;
      *tid = resources[guid].tid;
    } else 
      /* !!! setting exceptions does not work !!! */
      //CORBA_exception_set( _env, ex_IEarm_Manager_unknown_resource, NULL );
      L4_KDB_Enter("EARM open: unknown resource");

    return;
}

IDL4_PUBLISH_IEARM_MANAGER_OPEN(IEarm_Manager_open_implementation);

IDL4_INLINE void IEarm_Manager_close_implementation(CORBA_Object _caller, const guid_t guid, idl4_server_environment *_env)
{
    /* implementation of Iounting::Manager::close */
  
    L4_Word_t logid = tid_space_t::tid_to_space_id(_caller) + VM_LOGID_OFFSET;

    printf("EARM: close logid %d thread %t closes guid %d\n", logid, _caller, guid);
    L4_KDB_Enter("close");

    if( (guid < UUID_IEarm_ResMax) && (!L4_IsNilThread(resources[guid].tid)) ) {
      resources[guid].shared->clients[logid].limit = 0;
    } else
      CORBA_exception_set( _env, ex_IEarm_Manager_unknown_resource, NULL );


    return;
}

IDL4_PUBLISH_IEARM_MANAGER_CLOSE(IEarm_Manager_close_implementation);

void *IEarm_Manager_vtable[IEARM_MANAGER_DEFAULT_VTABLE_SIZE] = IEARM_MANAGER_DEFAULT_VTABLE;

void IEarm_Manager_server(
    void *param UNUSED,
    hthread_t *htread UNUSED)
{
  L4_ThreadId_t partner;
  L4_MsgTag_t msgtag;
  idl4_msgbuf_t msgbuf;
  long cnt;

  /* register with the locator */
  //printf("\tEARM: accounting manager register %d\n", UUID_IEarm_Manager);
  register_interface( UUID_IEarm_Manager, L4_Myself() );

  idl4_msgbuf_init(&msgbuf);
//  for (cnt = 0;cnt < IEARM_MANAGER_STRBUF_SIZE;cnt++)
//    idl4_msgbuf_add_buffer(&msgbuf, malloc(8000), 8000);

  while (1)
    {
      partner = L4_nilthread;
      msgtag.raw = 0;
      cnt = 0;

      while (1)
        {
          idl4_msgbuf_sync(&msgbuf);

          idl4_reply_and_wait(&partner, &msgtag, &msgbuf, &cnt);

          if (idl4_is_error(&msgtag))
            break;

          idl4_process_request(&partner, &msgtag, &msgbuf, &cnt, IEarm_Manager_vtable[idl4_get_function_id(&msgtag) & IEARM_MANAGER_FID_MASK]);
        }
    }
}

void IEarm_Manager_discard(void)
{
}


#define RPRINTF(x...)				\
    if (epruns >= 2) printf(x);
    
#define PRINT_HEADER()				\
    if (!printed_device)			\
    {						\
	RPRINTF("r%1u ", u);			\
	printed_device = true;			\
    }						\
    else					\
	RPRINTF(" ");				\
    if (!printed_logid)                         \
	RPRINTF("d%1u ", d);			\
    printed_logid = true;

static earm_set_t diff_set, old_set;
static L4_Word_t epruns;

static bool earmmanager_print_resource(L4_Word_t u, L4_Word_t ms)
{
    energy_t energy = 0;
    energy_t sum = 0;
    
    bool printed_device = false;

   
    for (L4_Word_t d = 0; d <= max_logid_in_use; d ++) 
    {
	bool printed_logid = false;

	/* Idle */
	for (L4_Word_t v = 0; v < UUID_IEarm_ResMax; v++)
	{
	    energy = resources[u].shared->clients[d].base_cost[v];
	    diff_set.res[u].clients[d].base_cost[v] = energy - old_set.res[u].clients[d].base_cost[v];
	    old_set.res[u].clients[d].base_cost[v] = energy;
	    diff_set.res[u].clients[d].base_cost[v] /= ms;
	    if (u == UUID_IEarm_ResDisk)
		diff_set.res[u].clients[d].base_cost[v] /= 1000;
            
            sum += diff_set.res[u].clients[d].base_cost[v];

	    if (diff_set.res[u].clients[d].base_cost[v])
	    {
		PRINT_HEADER();
 		RPRINTF("i%1u %4lu ", v, (L4_Word_t) diff_set.res[u].clients[d].base_cost[v]);
	    }
	}	    
        /* ess */
	for (L4_Word_t v = 0; v < UUID_IEarm_ResMax; v++)
	{
	    energy = resources[u].shared->clients[d].access_cost[v];
	    diff_set.res[u].clients[d].access_cost[v] = energy - old_set.res[u].clients[d].access_cost[v];
	    old_set.res[u].clients[d].access_cost[v] = energy;	
	    
	    diff_set.res[u].clients[d].access_cost[v] /= ms;
	    if (u == UUID_IEarm_ResDisk)
		diff_set.res[u].clients[d].access_cost[v] /= 1000;

            sum += diff_set.res[u].clients[d].access_cost[v];

	    if (diff_set.res[u].clients[d].access_cost[v])
	    {
		PRINT_HEADER();
		printed_logid = true;
		RPRINTF("a%1u %5lu ", v, (L4_Word_t) diff_set.res[u].clients[d].access_cost[v]);
	    }

	}
	if (printed_logid)
	    RPRINTF(" ");
        
       
    }
    
#if 0
    if (sum)
    {
        if (!printed_device)			
        {					
            RPRINTF("r %lu ", u);		
            printed_device = true;		
        }					
        RPRINTF("s %5lu ", (L4_Word_t) sum);
    }
#endif
    return printed_device;
   
}

void earmmanager_print_resources()
{
	L4_Clock_t now = L4_SystemClock();
	static L4_Clock_t last = { raw: 0 };
	L4_Word64_t ms = now.raw - last.raw;
	ms /= 1000;
	last = now;

        bool printed = false;
	for (L4_Word_t uuid_cpu = 0; uuid_cpu <= max_uuid_cpu; uuid_cpu++)
            printed |= earmmanager_print_resource(uuid_cpu, ms);
        printed |= earmmanager_print_resource(UUID_IEarm_ResDisk, ms);
	if (printed)
	    RPRINTF("\n");
	
	epruns++;
}

void earmmanager_print(
    void *param UNUSED,
    hthread_t *htread UNUSED)
{
    L4_Time_t sleep = L4_TimePeriod( EARM_MGR_PRINT_MSEC * 1000 );

    for (L4_Word_t d = 0; d < L4_LOG_MAX_LOGIDS; d++)
	for (L4_Word_t u = 0; u < UUID_IEarm_ResMax; u++)
	    for (L4_Word_t v = 0; v < UUID_IEarm_ResMax; v++)
	    {
		old_set.res[u].clients[d].base_cost[v] = 
		    diff_set.res[u].clients[d].base_cost[v] = 0;
		old_set.res[u].clients[d].access_cost[v] = 
		    diff_set.res[u].clients[d].access_cost[v] = 0;
	    }
    
    L4_Sleep(sleep);

    while (1) {
	earmmanager_print_resources();
	L4_Sleep(sleep);
    }
}

void earmmanager_init()
{
    
    
    for (L4_Word_t u = 0; u < UUID_IEarm_ResMax; u++)
    {
        resources[u].tid = L4_nilthread;
	for (L4_Word_t i = 0; i < LOGFILE_SIZE / sizeof(L4_Word_t) ; i++)
	    res_logfile[u][i] = 0;
    }

    /* Start resource manager thread */
    earmmanager_thread = get_hthread_manager()->create_thread( 
	hthread_idx_earmmanager, 252, false, IEarm_Manager_server);
    
    if( !earmmanager_thread )
    {
	printf("EARM: couldn't start accounting manager");
	L4_KDB_Enter();
	return;
    }
    printf("\tEARM manager TID: %t\n", earmmanager_thread->get_global_tid());

    earmmanager_thread->start();

#if defined(EARM_MGR_PRINT)
    if (!l4_pmsched_enabled)
    {
	/* Start printer */
	hthread_t *earmmanager_print_thread = get_hthread_manager()->create_thread( 
	    hthread_idx_earmmanager_print, 252, false,
	    earmmanager_print);

	if( !earmmanager_print_thread )
	{
	    printf("EARM: couldn't accounting manager printger");
	    L4_KDB_Enter();
	    return;
	}
	printf("\tEARM manager print TID: %t\n", earmmanager_print_thread->get_global_tid());

	earmmanager_print_thread->start();
    }
#endif   

}

void earmcpu_register( L4_ThreadId_t tid, L4_Word_t uuid_cpu, IEarm_shared_t **shared )
{
  CORBA_Environment env = idl4_default_environment;
  
  IEarm_Manager_register_resource( earmmanager_thread->get_global_tid(), uuid_cpu, 0, &env );
  ASSERT( env._major != CORBA_USER_EXCEPTION );
  
  *shared = resources[uuid_cpu].shared;
  //printf("EARM: register cpu shared %p resources[%d].shared %p\n", *shared,
  //     uuid_cpu, resources[uuid_cpu].shared);
}

