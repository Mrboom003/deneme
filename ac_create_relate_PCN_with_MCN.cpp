
/* ================================================================================================================================================
* File Name: ac-create-relate_PCN_with_MCN.cpp
*
* Description:
*   This Handler create  the PCN and relate with  MCN 
*
*
* Functions:
*  ac_create_relate_PCN_with_MCN(EPM_action_message_t msg)
*
*
* ===================================================================================================================================================
* Modification History
* ----------------------------------------------------------------------
*    Date             Name         Description of Change
*  ---------------------------------------------------------------------
*  21-APR-2021    Senthil Nathan   Initial Creation
* ===================================================================================================================================================*/

/* ===================================================================
* This handler can be placed at the “Start" and "Complete" Action of the Root task, Do Task, Conditional Task,
*  select-signoff Task, perform-signoff Task, Acknowledge and Review Task.
*
*
*   The handler shall accept following arguments
* 
* Input Arguments:
* 
* Name:                      Description:
* --------------             -----------------------------
*
* -Target                     Target Type
* -Secondary_object           Secondary object type
* -Relation                   Secondary relation for MCN
* -status_name                status to check 
* -To_attach_relation         To attach relation
* -secondary_relation         Secondary Relation
* -attribute_name             Attribute_name of MCN
* -pcn_type                   PCN type
* -check_wrk_name             PCN work flow name 
* -status_names_list          check and carrying from secondary object 
*
*
*
* Output Arguments:
*
* Name:                      Description:
* --------------             -----------------------------
* ifail, ITK_ok              Status
* =================================================================== */


#include<ac_custom_arcelik_function.h>
#include<ac_common.h>
#include<epm\epm.h>
#include<sa\sa.h>
#define  STATUS_NAME           "name"


boolean ar_check_if_workflow_inactive (tag_t rev,const char* wrk_name){
	tag_t* list = NULLTAG;
	int count = 0,
		iStatus = ITK_ok;
	tag_t tCRRootTask =NULLTAG;
	const char* __function__ = "ACCommon::ar_get_previous_revision_with_status";
	ARCELIK_TRACE_ENTER();

	try 
	{
		EPM_ask_active_job_for_target(rev,&count,&list);
		for (int i=0; i<count ;i++){
			char* str = NULL;
			EPM_ask_root_task(list[i], &tCRRootTask);
			AOM_ask_name(tCRRootTask, &str);

			if (tc_strcasecmp(str,wrk_name)==0){
				MEM_free(list);
				return false;}

		}
		MEM_free(list);
	}catch(...) {

		if(iStatus == ITK_ok)
		{
			TC_write_syslog("%s: Unhandled Exception",__function__);
			iStatus = ARCELIK_UNKNOWN_ERROR;
		}
	}
	ARCELIK_TRACE_LEAVE(iStatus);
	return true;
} 


int ac_create_relate_PCN_with_MCN(EPM_action_message_t msg)
{
	ACCommon common;
	ACHandler handler;
	int iCount                  =     0,
		iRelation_Count         =     0,
		iStatus                 =     0,
		iActive_Job_Count       =     0,
		iRev_Count              =     0,
		iStatus_Count           =     0;

	char* cMcn_Type             =     NULL,
		*cProduct_Type          =     NULL,
		*cMcn_Pda_value         =     NULL,
		*cActive_Wrokflow_name  =     NULL,
		*cObject_Name           =     NULL,
		*cStatus_Name           =     NULL;



	string sTarget_Primary_Name =      "",
		sSecondary_Name         =      "",
		sRelation               =      "",
		sTo_Attach_Relation     =      "",
		sAttribute_Name         =      "",
		sStatus_Name            =      "",
		sPcn_Type               =      "",
		sabort                  =      "",
		sCheck_Work_Name        =      "";


	/////




	tag_t tPcn_Relation_Tag    =         NULLTAG,
		*tAttchment            =        (tag_t*)0,
		*tSecondary_Object     =        (tag_t*)0,
		*tRev_Tag              =        (tag_t*)0,
		*tActive_Wrokflows     =        (tag_t*)0,
		*tstatus_list          =        (tag_t*)0,
		tcreated_status        =        NULLTAG,
		tProduct               =        NULLTAG,
		tProduct_Relation_Tag  =        NULLTAG,
		tPcn_Tag               =        NULLTAG,
		tPre_Rev               =        NULLTAG,
		tPcn_Rev                =       NULLTAG,
		tActive_Workflow_Root_Task=     NULLTAG,
		tTask_Owner				=		NULLTAG;


	bool bIsAllowedStatus ;
	bool bStatus_check;
	bool bcheck_revison;
	boolean  workflow_exit;
	map<string,string> mapArg;
	vector<string> vstatus_names_list,
		Vcarry_staus_next,
		Vtemp_status,
		vecTargetAttribute;

	const char* __function__ = "ac_create_relate_PCN_with_MCN";
	ARCELIK_TRACE_ENTER();
	try {
		mapArg = handler.ac_get_arguments(msg);

		if (mapArg["Target"] != ""){
			sTarget_Primary_Name=mapArg["Target"];
		}
		if (mapArg["Secondary_object"] != ""){
			sSecondary_Name=mapArg["Secondary_object"];
		}
		if (mapArg["Relation"] != ""){
			sRelation =mapArg["Relation"]; 
		}
		if (mapArg["To_attach_relation"] != ""){
			sTo_Attach_Relation=mapArg["To_attach_relation"];
		}
		if (mapArg["attribute_name"] != ""){
			vecTargetAttribute = handler.ar_split_string(mapArg["attribute_name"], '=');
		}

		if (mapArg["status_name"] != ""){
			sStatus_Name=mapArg["status_name"];
		}
		if (mapArg["pcn_type"] != ""){
			sPcn_Type=mapArg["pcn_type"];
		}
		if (mapArg["abort_workflow"] != ""){
			sabort=mapArg["abort_workflow"];
		}
		if (mapArg["check_wrk_name"] != ""){
			sCheck_Work_Name=mapArg["check_wrk_name"];
		}
		if (mapArg["carry_status_previous"] != "") { 

			vstatus_names_list = handler.ar_split_string(mapArg["carry_status_previous"], ',');
		}
		handler.ar_get_attachments(msg.task,EPM_target_attachment, & iCount, & tAttchment);
		ARCELIK_TRACE_CALL(iStatus=EPM_ask_responsible_party(msg.task,&tTask_Owner),AR_LOG_ERROR_AND_THROW);
		


		
		
		if (mapArg["carry_staus_next"] != "") { 

			Vcarry_staus_next = handler.ar_split_string(mapArg["carry_staus_next"], ',');
		}
		for (int p = 0; p < iCount ; p++){//att
			common.ar_get_class_name(tAttchment[p] , &cMcn_Type);
			if (tc_strcasecmp(sTarget_Primary_Name.c_str() , cMcn_Type) == 0){

				if(vecTargetAttribute.size() > 0)
				{
					ARCELIK_TRACE_CALL(iStatus = AOM_ask_value_string (tAttchment[p] , vecTargetAttribute[0].c_str() , &cMcn_Pda_value) , AR_LOG_ERROR_AND_THROW);
					if(tc_strcmp(cMcn_Pda_value, vecTargetAttribute[1].c_str()) != 0)
						continue;
					Custom_free(cMcn_Pda_value);

				}

				common.ar_get_secondary_objects (sRelation.c_str() , tAttchment[p],&iRelation_Count , &tSecondary_Object);
				
				for (int i = 0;i < iRelation_Count ; i++){

					common.ar_get_class_name(tSecondary_Object[i] , &cProduct_Type);
					cout<<"sSecondary_Name"<<sSecondary_Name<<cProduct_Type;
					if (tc_strcasecmp(sSecondary_Name.c_str() , cProduct_Type) == 0){
						cout<<"sabort"<<sabort;
						if(tc_strcasecmp(sabort.c_str() , AC_YES) == 0)
						{
							cout<<"entered";
							ARCELIK_TRACE_CALL(iStatus = ITEM_ask_item_of_rev(tSecondary_Object[i] , &tProduct) , AR_LOG_ERROR_AND_THROW);
							tPre_Rev = common.ar_get_previous_revision_with_status(tProduct , tSecondary_Object[i] , sStatus_Name.c_str() ,"" , &bIsAllowedStatus);
							if (tPre_Rev != NULLTAG){	
								ARCELIK_TRACE_CALL(iStatus = ITEM_list_all_revs(tProduct , &iRev_Count , &tRev_Tag) , AR_LOG_ERROR_AND_THROW);
								for (int i = iRev_Count-2;i >= 0 ; i--){
									if (!(ar_check_if_workflow_inactive(tRev_Tag[i] , sCheck_Work_Name.c_str()))){
										//  abort
										ARCELIK_TRACE_CALL(iStatus = EPM_ask_active_job_for_target(tRev_Tag[i] , &iActive_Job_Count , &tActive_Wrokflows), AR_LOG_ERROR_AND_THROW);
										for (int j =0 ; j < iActive_Job_Count ; j++){
											ARCELIK_TRACE_CALL(iStatus =EPM_ask_root_task(tActive_Wrokflows[j] , &tActive_Workflow_Root_Task), AR_LOG_ERROR_AND_THROW);
											ARCELIK_TRACE_CALL(iStatus =AOM_ask_name(tActive_Workflow_Root_Task , &cActive_Wrokflow_name), AR_LOG_ERROR_AND_THROW);
											if (tc_strcasecmp(cActive_Wrokflow_name,sCheck_Work_Name.c_str()) == 0){
												
												ARCELIK_TRACE_CALL(iStatus=EPM_assign_responsible_party(tActive_Workflow_Root_Task,tTask_Owner),AR_LOG_ERROR_AND_THROW);
												ARCELIK_TRACE_CALL(iStatus =EPM_trigger_action(tActive_Workflow_Root_Task , EPM_abort_action , "DBA Action "), AR_LOG_ERROR_AND_THROW);
												break;
											}
											Custom_free(cActive_Wrokflow_name);	
										}
										Custom_free(tActive_Wrokflows);
										break;
									}
								}
								Custom_free(tRev_Tag);
							}
						}
						if(sPcn_Type != "")
						{
							ARCELIK_TRACE_CALL(iStatus = AOM_ask_name(tAttchment[p],&cObject_Name) ,  AR_LOG_ERROR_AND_THROW);
							string str (cObject_Name);
							Custom_free(cObject_Name);
							common.ar_create_items(sPcn_Type.c_str() , str , &tPcn_Tag);
							ARCELIK_TRACE_CALL(iStatus =ITEM_ask_latest_rev(tPcn_Tag , &tPcn_Rev), AR_LOG_ERROR_AND_THROW);
							common.ar_attach_on_relation(tPcn_Rev,tSecondary_Object[i] , sRelation.c_str() , &tProduct_Relation_Tag);
							common.ar_attach_on_relation(tAttchment[p],tPcn_Rev , sTo_Attach_Relation.c_str() , &tPcn_Relation_Tag);
							common.ar_intiate_workFlow(tPcn_Rev , sCheck_Work_Name.c_str());
						}
						if (vstatus_names_list.size() > 0){   

							for (int i = 0 ; i < iRelation_Count ; i++){

								ARCELIK_TRACE_CALL(iStatus =ITEM_ask_item_of_rev(tSecondary_Object[i] , &tProduct) , AR_LOG_ERROR_AND_THROW);
								tPre_Rev = common.ar_get_previous_revision_with_status(tProduct , tSecondary_Object[i] , sStatus_Name.c_str() , "" , &bIsAllowedStatus);
								if (tPre_Rev != NULLTAG){
									ARCELIK_TRACE_CALL(iStatus =ITEM_list_all_revs(tProduct, &iRev_Count, &tRev_Tag)  , AR_LOG_ERROR_AND_THROW);
									for (int j = iRev_Count-2;j >= 0 ; j--){	
										if (ar_check_if_workflow_inactive(tRev_Tag[j] , sCheck_Work_Name.c_str())){
											common.ar_check_workflow_exists (tRev_Tag[j] , sCheck_Work_Name.c_str() , &workflow_exit);  
											if (workflow_exit){
												ARCELIK_TRACE_CALL(iStatus = WSOM_ask_release_status_list(tRev_Tag[j] , &iStatus_Count , &tstatus_list) , AR_LOG_ERROR_AND_THROW);
												for (int k = 0 ; k < iStatus_Count ; k++){
													ARCELIK_TRACE_CALL(iStatus = AOM_ask_value_string(tstatus_list[k] , STATUS_NAME , &cStatus_Name) , AR_LOG_ERROR_AND_THROW);
													for (int l =0 ; l<vstatus_names_list.size() ; l++){
														if (tc_strcasecmp(cStatus_Name , vstatus_names_list[l].c_str()) == 0){
															bStatus_check = FALSE;
															common.ar_check_status(tSecondary_Object[i],cStatus_Name,&bStatus_check);
															if (!bStatus_check){
																ARCELIK_TRACE_CALL(RELSTAT_create_release_status(cStatus_Name , &tcreated_status) , AR_LOG_ERROR_AND_THROW);
																if (tcreated_status != NULLTAG){
																	ARCELIK_TRACE_CALL(iStatus = RELSTAT_add_release_status(tcreated_status , 1 , &tSecondary_Object[i], FALSE), AR_LOG_ERROR_AND_THROW);
																	break;
																}
															}
														}
													}//l
													Custom_free(cStatus_Name);
												}
												Custom_free(tstatus_list);
												break;
											}
										}	
										else {
											break;
										}
									}
									Custom_free(tRev_Tag);
								}
							} 
						}
						if (Vcarry_staus_next.size() > 0){
							ARCELIK_TRACE_CALL(iStatus = WSOM_ask_release_status_list(tSecondary_Object[i] , &iStatus_Count , &tstatus_list) , AR_LOG_ERROR_AND_THROW);
							for (int k = 0 ; k < iStatus_Count ; k++){
								ARCELIK_TRACE_CALL(iStatus = AOM_ask_value_string(tstatus_list[k] , STATUS_NAME , &cStatus_Name) , AR_LOG_ERROR_AND_THROW);
								for (int f = 0 ; f<Vcarry_staus_next.size() ; f++){
									if (tc_strcasecmp(Vcarry_staus_next[f].c_str(),cStatus_Name)==0){
										Vtemp_status.push_back(cStatus_Name);
										Custom_free(cStatus_Name);
										break;
									}
								}
								Custom_free(cStatus_Name);
							}
							Custom_free(tstatus_list);

							ARCELIK_TRACE_CALL(iStatus = ITEM_ask_item_of_rev (tSecondary_Object[i] , &tProduct) , AR_LOG_ERROR_AND_THROW);
							ARCELIK_TRACE_CALL(iStatus =ITEM_list_all_revs(tProduct, &iRev_Count, &tRev_Tag)  , AR_LOG_ERROR_AND_THROW);
							bcheck_revison = FALSE;
							for (int b = 0 ; b<iRev_Count ; b++){
								if (tRev_Tag[b] == tSecondary_Object[i]){
									bcheck_revison = TRUE;
									continue;
								}
								if (bcheck_revison){
									ARCELIK_TRACE_CALL(iStatus = WSOM_ask_release_status_list(tRev_Tag[b] , &iStatus_Count , &tstatus_list) , AR_LOG_ERROR_AND_THROW);//statuslist
									for (int c = 0 ; c < iStatus_Count ; c++){
										ARCELIK_TRACE_CALL(iStatus = AOM_ask_value_string(tstatus_list[c] , STATUS_NAME , &cStatus_Name) , AR_LOG_ERROR_AND_THROW);//name
										if (tc_strcasecmp(cStatus_Name,sStatus_Name.c_str()) == 0){
											for (int e = 0; e < Vtemp_status.size() ; e++ ){
												bStatus_check = FALSE;
												common.ar_check_status(tRev_Tag[b] , Vtemp_status[e] , &bStatus_check);
												if (!bStatus_check){
													ARCELIK_TRACE_CALL(RELSTAT_create_release_status(Vtemp_status[e].c_str() , &tcreated_status) , AR_LOG_ERROR_AND_THROW);
													if (tcreated_status != NULLTAG){
														ARCELIK_TRACE_CALL(iStatus = RELSTAT_add_release_status(tcreated_status , 1 , &tRev_Tag[b], FALSE), AR_LOG_ERROR_AND_THROW);
													}
												}
											}
											Custom_free(cStatus_Name);
											break;
										}
										Custom_free(cStatus_Name);
									}
									Custom_free(tstatus_list);
									if (sStatus_Name==""){
										for (int e = 0; e < Vtemp_status.size() ; e++ ){
											bStatus_check = FALSE;
											common.ar_check_status(tRev_Tag[b] , Vtemp_status[e] , &bStatus_check);
											if (!bStatus_check){
												ARCELIK_TRACE_CALL(RELSTAT_create_release_status(Vtemp_status[e].c_str() , &tcreated_status) , AR_LOG_ERROR_AND_THROW);
												if (tcreated_status != NULLTAG){
													ARCELIK_TRACE_CALL(iStatus = RELSTAT_add_release_status(tcreated_status , 1 , &tRev_Tag[b], FALSE), AR_LOG_ERROR_AND_THROW);
												}
											}
										}

									}
								}

							}
							Custom_free(tRev_Tag);
							Vtemp_status.clear();
						}

					}

					Custom_free(cProduct_Type);
				}
				Custom_free(tSecondary_Object);
			}
			Custom_free(cMcn_Type);
		}
		Custom_free(tAttchment);
	}catch(...) {

		cout << "##### In catch block for Create Relate PCN with MCN \n";
		cout << "#### iStatus ::  " << iStatus << endl;
		if (iStatus == ITK_ok) {
			TC_write_syslog("\n %s: Unhandled Exception At LINE:: %d",
				__function__, __LINE__);
			iStatus = ARCELIK_UNKNOWN_ERROR;
		}
		TC_write_syslog("\n %s: Exception Error", __function__);
	}



	ARCELIK_TRACE_LEAVE(iStatus);
	return iStatus;
}

