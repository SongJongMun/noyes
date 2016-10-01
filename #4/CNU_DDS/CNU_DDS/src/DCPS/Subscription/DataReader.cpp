#include "../../../include/DCPS/Subscription/DataReader.h"
#include "../../../include/DCPS/Subscription/Subscriber.h"
#include "../../../include/Status/EntityStatus/DataReaderStatus.h"
#include "../../../include/Status/Condition/StatusCondition.h"

namespace CNU_DDS
{
	DataReader::DataReader()
	{
		status				= new DataReaderStatus();
		status_condition	= new StatusCondition();

		status_condition->enabled_statuses	= 0x00;
		status_condition->related_entity	= this;

		is_enabled			= false;
		listener			= NULL;
		qos					= NULL;
	}

	DataReader::~DataReader()
	{
		delete(status_condition);
		delete(status);
	}

	ReturnCode_t		DataReader::get_qos(DataReaderQos* qos_list)
	{
		ReturnCode_t	result	= RETCODE_OK;

		if(qos)
		{
			memcpy(qos_list, qos, sizeof(DataReaderQos));
			result		= RETCODE_OK;
		}
		else
		{
			result		= RETCODE_PRECONDITION_NOT_MET;	// undefined
		}

		return	result;
	}

	// ���� �̿Ϸ�
	// Immutable, Inconsistent Qos ��� ����ؾ� �ϹǷ� �켱 ����
	ReturnCode_t		DataReader::set_qos(DataReaderQos* qos_list)
	{
		ReturnCode_t	result	= RETCODE_OK;

		if(!is_enabled && qos == NULL)
		{
			qos	= new DataReaderQos();
			memcpy(qos, qos_list, sizeof(DataReaderQos));
			result	= RETCODE_OK;
			return result;
		}

		return	result;
	}

	Listener*			DataReader::get_listener(void)
	{
		Listener*	result	= NULL;


		result		= listener;
		return	result;
	}

	ReturnCode_t		DataReader::set_listener(Listener* a_listener, StatusMask mask)
	{
		ReturnCode_t	result	= RETCODE_OK;

		if(listener!=NULL && a_listener == NULL)
		{
			delete(listener);
			listener	= NULL;
			status_condition->set_enabled_statuses(mask);
			result		= RETCODE_OK;
		}
		else
		{
			listener	= a_listener;
			status_condition->set_enabled_statuses(mask);
			result		= RETCODE_OK;
		}

		result		= RETCODE_ERROR;
		return	result;
	}

	ReturnCode_t		DataReader::enable(void)
	{
		ReturnCode_t	result	= RETCODE_OK;

		is_enabled 		= true;
		result				= RETCODE_OK;

		return	result;
	}

	ReturnCode_t		DataReader::read(DataList* data_values, SampleInfoList* sample_infos, long max_sample, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states)
	{
		ReturnCode_t	result	= RETCODE_OK;

		if(!is_enabled)
		{
			result		=	RETCODE_NOT_ENABLED;
			return	result;
		}

		if(max_sample == 0)		// Zero-copy�� ��� return_loan�� ȣ���ؾ� �� , �� �� ���� �ʿ�
		{
			result		= return_loan(data_values, sample_infos);
			return 	result;
		}

		read_flag	 = true; // READING - Subscriber���� �����͸� �д� ������ �Ǻ��ϱ� ���ؼ� �߰�.

		int							i		= 0;
		Node<DataSample>* node = data_sample_list.getNodeByIndex(0);


		if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == INSTANCE_PRESENTATION_QOS)
		{
			while(data_sample_list.getSize() > i++)
			{
				node = node->rear;

				// �Ѿ���� �Ķ���� 3���� ������ ��� �����ϴ� Data, SampleInfo�� ��� �����͸� ����
				if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
						(node->value->info->instance_state & instance_states))
				{
					if(max_sample > data_values->getSize()) // �ִ� �����ŭ �����͸� ����
					{
						node->value->info->sample_state	= READ_SAMPLE_STATE;
						node->value->info->view_state		= NOT_NEW_VIEW_STATE;

						data_values->insertInRear(node->value->value);
						sample_infos->insertInRear(node->value->info);
					}
					else // ���� ��ŭ ��� �о����� while���� Ż��... ���� Ż�� ���� RTPS�� �̿��ؼ� ������ �κа� return ó���� ��
					{
						break;
					}
				}
			}
		}
		else if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == TOPIC_PRESENTATION_QOS)
		{
			if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.ordered_access == true)
			{
				if(((DataReaderQos*)qos)->destination_order.kind == BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS)
				{
					while(data_sample_list.getSize() > i++)
					{
						node = node->rear;

						if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
								(node->value->info->instance_state & instance_states))
						{
							if(max_sample > data_values->getSize())
							{
								node->value->info->sample_state	= READ_SAMPLE_STATE;
								node->value->info->view_state		= NOT_NEW_VIEW_STATE;

								data_values->insertInRear(node->value->value);
								sample_infos->insertInRear(node->value->info);
							}
							else
							{
								break;
							}
						}
					}
				}
				else // Source_timestamp�� ����Ͽ� List�� ����
				{
					Time_t	cTime;
					bool		cFlag;
					int			index;
					int			max_len;
					int			j;

					cTime.sec			= 0;
					cTime.nanosec		= 0;

					// �ּҰ��� ã��
					while(data_sample_list.getSize() > i++)
					{
						node = node->rear;

						// State�� �����ϴ� ������ �� TimeStamp�� ����ؾ� ��
						if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
								(node->value->info->instance_state & instance_states))
						{
							if((cTime.sec == 0) && (cTime.nanosec == 0))
							{
								cTime.sec			= node->value->info->source_timestamp.sec;
								cTime.nanosec		= node->value->info->source_timestamp.nanosec;

								index	= i;
								max_len++;
							}
							else if(cTime.sec > node->value->info->source_timestamp.sec)
							{
								cTime.sec			= node->value->info->source_timestamp.sec;
								cTime.nanosec		= node->value->info->source_timestamp.nanosec;

								index	= i;
								max_len++;
							}
							else if( (cTime.sec == node->value->info->source_timestamp.sec) && (cTime.nanosec > node->value->info->source_timestamp.nanosec))
							{
								cTime.sec			= node->value->info->source_timestamp.sec;
								cTime.nanosec		= node->value->info->source_timestamp.nanosec;

								index	= i;
								max_len++;
							}
						}
					}

					// ���� ���� �����Ͱ� ����ִ� Index�� ��带 �̵����Ѽ� �ش� ����� �����͸� ����
					node	=	data_sample_list.getNodeByIndex(index);

					node->value->info->sample_state	= READ_SAMPLE_STATE;
					node->value->info->view_state		= NOT_NEW_VIEW_STATE;

					data_values->insertInRear(node->value->value);
					sample_infos->insertInRear(node->value->info);

					// ������ ���� �� �ִ� �������� ũ�⺸�� ���ѵ� ũ�⺸�� ũ�ٸ�
					// �ִ� ũ��� 5�ε� ���� �� �ִ� �����Ͱ� �̺��� �۴ٸ� �ִ� ũ�⸸ŭ �����͸� ���� �� �����Ƿ� �ǹ̾��� ���� ������ �ʾƵ� ��

					if(max_sample <= max_len)
					{
						max_len = max_sample;
					}

					//int		dup_data[max_len];
					int*	dup_data	= new int[max_len];
					int		k;
					dup_data[0] = index;

					cFlag 	= true;
					i			= 0;
					j			= 0;

					cTime.sec			= 0;
					cTime.nanosec		= 0;

					// ���� ���� �� ���� ũ���� ������ �߿��� ���� ���� ������ ���ʷ� List�� ����
					while(max_len-1 > j++)
					{
						node		=  data_sample_list.getNodeByIndex(0);
						cFlag		=  true;

						while(data_sample_list.getSize() > i++)
						{
							node = node->rear;
							if((node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
									(node->value->info->instance_state & instance_states))
							{
								for(k = 0; k < j; k++) // ������ �����ߴ� Index�� ��ģ�ٸ� �ߺ��Ǵ� ���̹Ƿ� üũ, Flag���� True��� ��ġ�� �ʴ� �����Ͷ�� �ǹ�
								{
									if(dup_data[k] == i)
									{
										cFlag = false;
										break;
									}
								}

								if((cTime.sec == 0) && (cTime.nanosec == 0) && cFlag)
								{
									cTime.sec			= node->value->info->source_timestamp.sec;
									cTime.nanosec		= node->value->info->source_timestamp.nanosec;

									dup_data[j] = i;
								}
								else if(cTime.sec > node->value->info->source_timestamp.sec && cFlag)
								{
									cTime.sec			= node->value->info->source_timestamp.sec;
									cTime.nanosec		= node->value->info->source_timestamp.nanosec;

									dup_data[j] = i;
								}
								else if( (cTime.sec == node->value->info->source_timestamp.sec) && (cTime.nanosec > node->value->info->source_timestamp.nanosec) && cFlag)
								{
									cTime.sec			= node->value->info->source_timestamp.sec;
									cTime.nanosec		= node->value->info->source_timestamp.nanosec;

									dup_data[j] = i;
								}
							}
						}

						node	=	data_sample_list.getNodeByIndex(dup_data[j]);

						node->value->info->sample_state	= READ_SAMPLE_STATE;
						node->value->info->view_state		= NOT_NEW_VIEW_STATE;

						data_values->insertInRear(node->value->value);
						sample_infos->insertInRear(node->value->info);
					}
				}
			}
			else // Ordered_access = false
			 {
				while(data_sample_list.getSize() > i++)
				{
					node = node->rear;

					if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
							(node->value->info->instance_state & instance_states))
					{
						if(max_sample > data_values->getSize())
						{
							node->value->info->sample_state	= READ_SAMPLE_STATE;
							node->value->info->view_state		= NOT_NEW_VIEW_STATE;

							data_values->insertInRear(node->value->value);
							sample_infos->insertInRear(node->value->info);
						}
						else
						{
							break;
						}
					}
				}
			}
		}
		else if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == GROUP_PRESENTATION_QOS)
		{
			// presentation qos�� group�̸� ordered_access�� true�� ��쿡�� �ִ� 1���� ������ ��ȯ�Ѵ�.
			 if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.ordered_access == true)
			 {
				 if(((DataReaderQos*)qos)->destination_order.kind == BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS)
				 {
					 while(data_sample_list.getSize() > i++)
					 {
						 node = node->rear;

						 if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
								 (node->value->info->instance_state & instance_states))
						 {
							 node->value->info->sample_state	= READ_SAMPLE_STATE;
							 node->value->info->view_state		= NOT_NEW_VIEW_STATE;

							 data_values->insertInRear(node->value->value);
							 sample_infos->insertInRear(node->value->info);

							 break;
						 }
					 }
				 }
				 else // destination_order.kind == SOURCE_TIMESTAMP
				 {
					 Time_t 	cTime;
					 int		count; // Data, SampleInfo �ϳ��� ������ �ǹǷ� �迭�� �̿����� ����

					 // ��������� �ý��� �����ϰ� 0�ʿ� �����͸� �� �� �����״� 0,0���� �ʱ�ȭ �Ͽ���
					 cTime.sec			= 0;
					 cTime.nanosec	= 0;

					 // ���� ���� ���� ã��
					 while(data_sample_list.getSize() > i++)
					 {
						 node = node->rear;

						 if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
								 (node->value->info->instance_state & instance_states))
						 {
							 if((cTime.sec == 0) && (cTime.nanosec == 0))
							 {
								 cTime.sec			= node->value->info->source_timestamp.sec;
								 cTime.nanosec	= node->value->info->source_timestamp.nanosec;
								 count				= i;
							 }
							 else if(cTime.sec > node->value->info->source_timestamp.sec)
							 {
								 cTime.sec			= node->value->info->source_timestamp.sec;
								 cTime.nanosec	= node->value->info->source_timestamp.nanosec;
								 count				= i;
							 }
							 else if(cTime.sec == node->value->info->source_timestamp.sec)
							 {
								 if(cTime.nanosec > node->value->info->source_timestamp.nanosec)
								 {
									 cTime.sec			= node->value->info->source_timestamp.sec;
									 cTime.nanosec	= node->value->info->source_timestamp.nanosec;
									 count				= i;
								 }
							 }
						 }
					 }

					 node = data_sample_list.getNodeByIndex(count);

					 node->value->info->sample_state	= READ_SAMPLE_STATE;
					 node->value->info->view_state		= NOT_NEW_VIEW_STATE;

					 data_values->insertInRear(node->value->value);
					 sample_infos->insertInRear(node->value->info);
				 }
			 }
			 else // Ordered_access = false
			 {
				 while(data_sample_list.getSize() > i++)
				 {
					 node = node->rear;

					 if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
							 (node->value->info->instance_state & instance_states))
					 {
						 if(max_sample > data_values->getSize())
						 {
							 node->value->info->sample_state	= READ_SAMPLE_STATE;
							 node->value->info->view_state		= NOT_NEW_VIEW_STATE;

							 data_values->insertInRear(node->value->value);
							 sample_infos->insertInRear(node->value->info);
						 }
						 else
						 {
							 break;
						 }
					 }
				 }
			 }
		}

		read_flag	= false;

		// �����͸� ������ ����Ʈ�� ũ�Ⱑ 0���� ũ�ٸ� �����͸� ���� ����.. ������ ���ʿ� �Ѿ���� ����Ʈ�� ũ�Ⱑ 0�̶�� ���� ������ �� �����Ƿ� ����� �ʿ���
		// �����ϰ� �ϸ� ���ʿ� ����Ʈ ũ�⸦ �����Ͽ��ٰ� ���ϰų� �����͸� �д� ���� result = RETCODE_OK�� �ִ� ����� �ְ���..
		if(data_values->getSize() > 0)
		{
			result		= RETCODE_OK;
		}
		else // �����͸� ã�� ��������(�ƿ� �����͸� �ϳ��� ���� ���� ���)
		{
			result		 = RETCODE_NO_DATA;
		}

		// rtps ���� �ʿ�
		// ���⼭ sample_info�� �پ���?? rank ���� ��������� ��
	}

	// read ���� �޾� ���� �Ķ���Ͱ����� ReadCondition���� ��ü�ϴ� �� ����
	// a_condition���� �޾ƿ��� state ������ get �Լ��� �ؾ� �ϴµ� ������ �ȵ��־ ���� ���Ƿ� �켱�� �׳� ���� �������� ��������
	ReturnCode_t		DataReader::read_w_condition(DataList* data_values, SampleInfoList* sample_infos, long max_sample, ReadCondition* a_condition)
	{
		ReturnCode_t	result	= RETCODE_OK;

		if(!is_enabled)
		{
			result		=	RETCODE_NOT_ENABLED;
			return	result;
		}

		// get_datareader�� �̱��� �����̹Ƿ� ������ ���� �ּ�ó�� ��
		/*
		if(a_condition->get_datareader() != this)
		{
			result		= RETCODE_PRECONDITION_NOT_MET;
			return	result;
		}
		*/

		if(max_sample == 0)		// Zero-copy�� ��� return_loan�� ȣ���ؾ� �� , �� �� ���� �ʿ�
		{
			result		= return_loan(data_values, sample_infos);
			return 	result;
		}

		read_flag	 = true; // READING - Subscriber���� �����͸� �д� ������ �Ǻ��ϱ� ���ؼ� �߰�.

		int							i		= 0;
		Node<DataSample>* node = data_sample_list.getNodeByIndex(0);


		if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == INSTANCE_PRESENTATION_QOS)
		{
			while(data_sample_list.getSize() > i++)
			{
				node = node->rear;

				if( (node->value->info->sample_state & a_condition->sample_state_mask) && (node->value->info->view_state & a_condition->view_state_mask) &&
						(node->value->info->instance_state & a_condition->instance_state_mask))
				{
					if(max_sample > data_values->getSize())
					{
						node->value->info->sample_state	= READ_SAMPLE_STATE;
						node->value->info->view_state		= NOT_NEW_VIEW_STATE;

						data_values->insertInRear(node->value->value);
						sample_infos->insertInRear(node->value->info);
					}
					else
					{
						break;
					}
				}
			}
		}
		else if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == TOPIC_PRESENTATION_QOS)
		{
			if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.ordered_access == true)
			{
				if(((DataReaderQos*)qos)->destination_order.kind == BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS)
				{
					while(data_sample_list.getSize() > i++)
					{
						node = node->rear;

						if( (node->value->info->sample_state & a_condition->sample_state_mask) && (node->value->info->view_state & a_condition->view_state_mask) &&
								(node->value->info->instance_state & a_condition->instance_state_mask))
						{
							if(max_sample > data_values->getSize())
							{
								node->value->info->sample_state	= READ_SAMPLE_STATE;
								node->value->info->view_state		= NOT_NEW_VIEW_STATE;

								data_values->insertInRear(node->value->value);
								sample_infos->insertInRear(node->value->info);
							}
							else
							{
								break;
							}
						}
					}
				}
				else // Source_timestamp�� ����Ͽ� List�� ����
				{
					Time_t	cTime;
					bool		cFlag;
					int			index;
					int			max_len;
					int			j;

					cTime.sec			= 0;
					cTime.nanosec		= 0;

					// �ּҰ��� ã��
					while(data_sample_list.getSize() > i++)
					{
						node = node->rear;

						// State�� �����ϴ� ������ �� TimeStamp�� ����ؾ� ��
						if( (node->value->info->sample_state & a_condition->sample_state_mask) && (node->value->info->view_state & a_condition->view_state_mask) &&
								(node->value->info->instance_state & a_condition->instance_state_mask))
						{
							if((cTime.sec == 0) && (cTime.nanosec == 0))
							{
								cTime.sec			= node->value->info->source_timestamp.sec;
								cTime.nanosec		= node->value->info->source_timestamp.nanosec;

								index	= i;
								max_len++;
							}
							else if(cTime.sec > node->value->info->source_timestamp.sec)
							{
								cTime.sec			= node->value->info->source_timestamp.sec;
								cTime.nanosec		= node->value->info->source_timestamp.nanosec;

								index	= i;
								max_len++;
							}
							else if( (cTime.sec == node->value->info->source_timestamp.sec) && (cTime.nanosec > node->value->info->source_timestamp.nanosec))
							{
								cTime.sec			= node->value->info->source_timestamp.sec;
								cTime.nanosec		= node->value->info->source_timestamp.nanosec;

								index	= i;
								max_len++;
							}
						}
					}

					// ���� ���� �����Ͱ� ����ִ� Index�� ��带 �̵����Ѽ� �ش� ����� �����͸� ����
					node	=	data_sample_list.getNodeByIndex(index);

					node->value->info->sample_state	= READ_SAMPLE_STATE;
					node->value->info->view_state		= NOT_NEW_VIEW_STATE;

					data_values->insertInRear(node->value->value);
					sample_infos->insertInRear(node->value->info);

					// ������ ���� �� �ִ� �������� ũ�⺸�� ���ѵ� ũ�⺸�� ũ�ٸ�
					// �ִ� ũ��� 5�ε� ���� �� �ִ� �����Ͱ� �̺��� �۴ٸ� �ִ� ũ�⸸ŭ �����͸� ���� �� �����Ƿ� �ǹ̾��� ���� ������ �ʾƵ� ��

					if(max_sample <= max_len)
					{
						max_len = max_sample;
					}

					//int		dup_data[max_len];
					int*	dup_data	= new int[max_len];
					int		k;
					dup_data[0] = index;

					cFlag 	= true;
					i			= 0;
					j			= 0;

					cTime.sec			= 0;
					cTime.nanosec		= 0;

					// ���� ���� �� ���� ũ���� ������ �߿��� ���� ���� ������ ���ʷ� List�� ����
					while(max_len-1 > j++)
					{
						node		=  data_sample_list.getNodeByIndex(0);
						cFlag		=  true;

						while(data_sample_list.getSize() > i++)
						{
							node = node->rear;
							if( (node->value->info->sample_state & a_condition->sample_state_mask) && (node->value->info->view_state & a_condition->view_state_mask) &&
									(node->value->info->instance_state & a_condition->instance_state_mask))
							{
								for(k = 0; k < j; k++) // ������ �����ߴ� Index�� ��ģ�ٸ� �ߺ��Ǵ� ���̹Ƿ� üũ, Flag���� True��� ��ġ�� �ʴ� �����Ͷ�� �ǹ�
								{
									if(dup_data[k] == i)
									{
										cFlag = false;
										break;
									}
								}

								if((cTime.sec == 0) && (cTime.nanosec == 0) && cFlag)
								{
									cTime.sec			= node->value->info->source_timestamp.sec;
									cTime.nanosec		= node->value->info->source_timestamp.nanosec;

									dup_data[j] = i;
								}
								else if(cTime.sec > node->value->info->source_timestamp.sec && cFlag)
								{
									cTime.sec			= node->value->info->source_timestamp.sec;
									cTime.nanosec		= node->value->info->source_timestamp.nanosec;

									dup_data[j] = i;
								}
								else if( (cTime.sec == node->value->info->source_timestamp.sec) && (cTime.nanosec > node->value->info->source_timestamp.nanosec) && cFlag)
								{
									cTime.sec			= node->value->info->source_timestamp.sec;
									cTime.nanosec		= node->value->info->source_timestamp.nanosec;

									dup_data[j] = i;
								}
							}
						}

						node	=	data_sample_list.getNodeByIndex(dup_data[j]);

						node->value->info->sample_state	= READ_SAMPLE_STATE;
						node->value->info->view_state		= NOT_NEW_VIEW_STATE;

						data_values->insertInRear(node->value->value);
						sample_infos->insertInRear(node->value->info);
					}
				}
			}
			else // Ordered_access = false
			{
				while(data_sample_list.getSize() > i++)
				{
					node = node->rear;

					if( (node->value->info->sample_state & a_condition->sample_state_mask) && (node->value->info->view_state & a_condition->view_state_mask) &&
							(node->value->info->instance_state & a_condition->instance_state_mask))
					{
						if(max_sample > data_values->getSize())
						{
							node->value->info->sample_state	= READ_SAMPLE_STATE;
							node->value->info->view_state		= NOT_NEW_VIEW_STATE;

							data_values->insertInRear(node->value->value);
							sample_infos->insertInRear(node->value->info);
						}
						else
						{
							break;
						}
					}
				}
			}
		}
		else if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == GROUP_PRESENTATION_QOS)
		{
			// presentation qos�� group�̸� ordered_access�� true�� ��쿡�� �ִ� 1���� ������ ��ȯ�Ѵ�.
			if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.ordered_access == true)
			{
				if(((DataReaderQos*)qos)->destination_order.kind == BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS)
				{
					while(data_sample_list.getSize() > i++)
					{
						node = node->rear;

						if( (node->value->info->sample_state & a_condition->sample_state_mask) && (node->value->info->view_state & a_condition->view_state_mask) &&
								(node->value->info->instance_state & a_condition->instance_state_mask))
						{
							node->value->info->sample_state	= READ_SAMPLE_STATE;
							node->value->info->view_state		= NOT_NEW_VIEW_STATE;

							data_values->insertInRear(node->value->value);
							sample_infos->insertInRear(node->value->info);

							break;
						}
					}
				}
				else // destination_order.kind == SOURCE_TIMESTAMP
				{
					Time_t 	cTime;
					int			count;

					// ��������� �ý��� �����ϰ� 0�ʿ� �����͸� �� �� �����״� 0,0���� �ʱ�ȭ �Ͽ���
					cTime.sec			= 0;
					cTime.nanosec	= 0;

					// ���� ���� ���� ã��
					while(data_sample_list.getSize() > i++)
					{
						node = node->rear;

						if( (node->value->info->sample_state & a_condition->sample_state_mask) && (node->value->info->view_state & a_condition->view_state_mask) &&
								(node->value->info->instance_state & a_condition->instance_state_mask))
						{
							if((cTime.sec == 0) && (cTime.nanosec == 0))
							{
								cTime.sec			= node->value->info->source_timestamp.sec;
								cTime.nanosec	= node->value->info->source_timestamp.nanosec;
								count				= i;
							}
							else if(cTime.sec > node->value->info->source_timestamp.sec)
							{
								cTime.sec			= node->value->info->source_timestamp.sec;
								cTime.nanosec	= node->value->info->source_timestamp.nanosec;
								count				= i;
							}
							else if(cTime.sec == node->value->info->source_timestamp.sec)
							{
								if(cTime.nanosec > node->value->info->source_timestamp.nanosec)
								{
									cTime.sec			= node->value->info->source_timestamp.sec;
									cTime.nanosec	= node->value->info->source_timestamp.nanosec;
									count				= i;
								}
							}
						}
					}

					node = data_sample_list.getNodeByIndex(count);

					node->value->info->sample_state	= READ_SAMPLE_STATE;
					node->value->info->view_state		= NOT_NEW_VIEW_STATE;

					data_values->insertInRear(node->value->value);
					sample_infos->insertInRear(node->value->info);
				}
			}
			else // Ordered_access = false
			{
				while(data_sample_list.getSize() > i++)
				{
					node = node->rear;

					if( (node->value->info->sample_state & a_condition->sample_state_mask) && (node->value->info->view_state & a_condition->view_state_mask) &&
							(node->value->info->instance_state & a_condition->instance_state_mask))
					{
						if(max_sample > data_values->getSize())
						{
							node->value->info->sample_state	= READ_SAMPLE_STATE;
							node->value->info->view_state		= NOT_NEW_VIEW_STATE;

							data_values->insertInRear(node->value->value);
							sample_infos->insertInRear(node->value->info);
						}
						else
						{
							break;
						}
					}
				}
			}
		}

		read_flag	= false;

		if(data_values->getSize() > 0)
		{
			result		= RETCODE_OK;
		}
		else
		{
			result		 = RETCODE_NO_DATA;
		}

		// rtps ���� �ʿ�
		// ���⼭ sample_info�� �پ���?? rank ���� ��������� ��
	}

	// ���� �������� ���� DataReader�� Data�� �����Ͽ� ������, SampleInfo ���� �����ؾ� ��
	// read Operation�� ��������� max_samples, Sample_state�� view_state, instance_state�� ������ ���� �ٸ� ��.
	// max_sample = 1, sample_state = NOT_READ, view_state = ANY_VIEW, instance_state = ANY_INSTANACE
	ReturnCode_t		DataReader::read_next_sample(DDS_Data* data_value, SampleInfo* sample_info)
	{
		ReturnCode_t	result	= RETCODE_OK;

		if(!is_enabled)
		{
			result		=	RETCODE_NOT_ENABLED;
			return	result;
		}

		read_flag	 = true; // READING - Subscriber���� �����͸� �д� ������ �Ǻ��ϱ� ���ؼ� �߰�.

		int							i		= 0;
		Node<DataSample>* node = data_sample_list.getNodeByIndex(0);


		if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == INSTANCE_PRESENTATION_QOS)
		{
			while(data_sample_list.getSize() > i++)
			{
				node = node->rear;

				if( (node->value->info->sample_state & NOT_READ_SAMPLE_STATE) && (node->value->info->view_state & ANY_VIEW_STATE) &&
						(node->value->info->instance_state & ANY_INSTANCE_STATE))
				{
					node->value->info->sample_state	= READ_SAMPLE_STATE;
					node->value->info->view_state		= NOT_NEW_VIEW_STATE;

					data_value = node->value->value;
					sample_info = node->value->info;

					break;
				}
			}
		}
		else if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == TOPIC_PRESENTATION_QOS)
		{
			if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.ordered_access == true)
			{
				if(((DataReaderQos*)qos)->destination_order.kind == BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS)
				{
					while(data_sample_list.getSize() > i++)
					{
						node = node->rear;

						if( (node->value->info->sample_state & NOT_READ_SAMPLE_STATE) && (node->value->info->view_state & ANY_VIEW_STATE) &&
								(node->value->info->instance_state & ANY_INSTANCE_STATE))
						{
								node->value->info->sample_state	= READ_SAMPLE_STATE;
								node->value->info->view_state		= NOT_NEW_VIEW_STATE;

								data_value = node->value->value;
								sample_info = node->value->info;

								break;
						}
					}
				}
				else // Source_timestamp�� ����Ͽ� List�� ����
				{
					Time_t 	cTime;
					bool 		cFlag;
					int			count;

					// ��������� �ý��� �����ϰ� 0�ʿ� �����͸� ��� ������ 0,0���� �ʱ�ȭ �Ͽ���
					cTime.sec			= 0;
					cTime.nanosec		= 0;

					// ���� ���� ���� ã��
					while(data_sample_list.getSize() > i++)
					{
						node = node->rear;

						if( (node->value->info->sample_state & NOT_READ_SAMPLE_STATE) && (node->value->info->view_state & ANY_VIEW_STATE) &&
								(node->value->info->instance_state & ANY_INSTANCE_STATE))
						{
							if((cTime.sec == 0) && (cTime.nanosec == 0))
							{
								cTime.sec			= node->value->info->source_timestamp.sec;
								cTime.nanosec		= node->value->info->source_timestamp.nanosec;
								count					= i;
							}
							else if(cTime.sec > node->value->info->source_timestamp.sec)
							{
								cTime.sec			= node->value->info->source_timestamp.sec;
								cTime.nanosec		= node->value->info->source_timestamp.nanosec;
								count					= i;
							}
							else if(cTime.sec == node->value->info->source_timestamp.sec)
							{
								if(cTime.nanosec > node->value->info->source_timestamp.nanosec)
								{
									cTime.sec			= node->value->info->source_timestamp.sec;
									cTime.nanosec		= node->value->info->source_timestamp.nanosec;
									count					= i;
								}
							}
						}
					}

					node = data_sample_list.getNodeByIndex(count);

					node->value->info->sample_state	= READ_SAMPLE_STATE;
					node->value->info->view_state		= NOT_NEW_VIEW_STATE;

					data_value = node->value->value;
					sample_info = node->value->info;
				}
			}
			else // false
			 {
				while(data_sample_list.getSize() > i++)
				{
					node = node->rear;

					if( (node->value->info->sample_state & NOT_READ_SAMPLE_STATE) && (node->value->info->view_state & ANY_VIEW_STATE) &&
							(node->value->info->instance_state & ANY_INSTANCE_STATE))
					{
						node->value->info->sample_state	= READ_SAMPLE_STATE;
						node->value->info->view_state		= NOT_NEW_VIEW_STATE;

						data_value = node->value->value;
						sample_info = node->value->info;

						break;
					}
				}
			}
		}
		else if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == GROUP_PRESENTATION_QOS)
		{
			// presentation qos�� group�̸� ordered_access�� true�� ��쿡�� �ִ� 1���� ������ ��ȯ�Ѵ�.
			 if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.ordered_access == true)
			 {
				 if(((DataReaderQos*)qos)->destination_order.kind == BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS)
				 {
					 while(data_sample_list.getSize() > i++)
					 {
						 node = node->rear;

							if( (node->value->info->sample_state & NOT_READ_SAMPLE_STATE) && (node->value->info->view_state & ANY_VIEW_STATE) &&
									(node->value->info->instance_state & ANY_INSTANCE_STATE))
						 {
							 node->value->info->sample_state	= READ_SAMPLE_STATE;
							 node->value->info->view_state		= NOT_NEW_VIEW_STATE;

							 data_value = node->value->value;
							 sample_info = node->value->info;

							 break;
						 }
					 }
				 }
				 else // destination_order.kind == SOURCE_TIMESTAMP
				 {
					 Time_t 	cTime;
					 int		count;

					 // ��������� �ý��� �����ϰ� 0�ʿ� �����͸� �� �� �����״� 0,0���� �ʱ�ȭ �Ͽ���
					 cTime.sec			= 0;
					 cTime.nanosec	= 0;

					 // ���� ���� ���� ã��
					 while(data_sample_list.getSize() > i++)
					 {
						 node = node->rear;

							if( (node->value->info->sample_state & NOT_READ_SAMPLE_STATE) && (node->value->info->view_state & ANY_VIEW_STATE) &&
									(node->value->info->instance_state & ANY_INSTANCE_STATE))
						 {
							 if((cTime.sec == 0) && (cTime.nanosec == 0))
							 {
								 cTime.sec			= node->value->info->source_timestamp.sec;
								 cTime.nanosec	= node->value->info->source_timestamp.nanosec;
								 count				= i;
							 }
							 else if(cTime.sec > node->value->info->source_timestamp.sec)
							 {
								 cTime.sec			= node->value->info->source_timestamp.sec;
								 cTime.nanosec	= node->value->info->source_timestamp.nanosec;
								 count				= i;
							 }
							 else if(cTime.sec == node->value->info->source_timestamp.sec)
							 {
								 if(cTime.nanosec > node->value->info->source_timestamp.nanosec)
								 {
									 cTime.sec			= node->value->info->source_timestamp.sec;
									 cTime.nanosec	= node->value->info->source_timestamp.nanosec;
									 count				= i;
								 }
							 }
						 }
					 }

					 node = data_sample_list.getNodeByIndex(count);

					 node->value->info->sample_state	= READ_SAMPLE_STATE;
					 node->value->info->view_state		= NOT_NEW_VIEW_STATE;

					 data_value = node->value->value;
					 sample_info = node->value->info;

				 }
			 }
			 else // Ordered_access = false
			 {
				 while(data_sample_list.getSize() > i++)
				 {
					 node = node->rear;

					 if( (node->value->info->sample_state & NOT_READ_SAMPLE_STATE) && (node->value->info->view_state & ANY_VIEW_STATE) &&
							 (node->value->info->instance_state & ANY_INSTANCE_STATE))
					 {
						 node->value->info->sample_state	= READ_SAMPLE_STATE;
						 node->value->info->view_state		= NOT_NEW_VIEW_STATE;

						 data_value = node->value->value;
						 sample_info = node->value->info;

						 break;
					 }
				 }
			 }
		}

		read_flag	= false;

		// OK���� NO_DATA���� �Ǻ��ϴ� ����� ���� ���..
		if(data_value != NULL)
		{
			result		= RETCODE_OK;
		}
		else
		{
			result		 = RETCODE_NO_DATA;
		}

		// rtps ���� �ʿ�
		// ���⼭ sample_info�� �پ���?? rank ���� ��������� ��
	}

	ReturnCode_t		DataReader::take(DataList* data_values, SampleInfoList* sample_infos, long max_sample, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states)
	{
		ReturnCode_t	result	= RETCODE_OK;

		if(!is_enabled)
		{
			result		=	RETCODE_NOT_ENABLED;
			return	result;
		}

		if(max_sample == 0)		// Zero-copy�� ��� return_loan�� ȣ���ؾ� �� , �� �� ���� �ʿ�
		{
			result		= return_loan(data_values, sample_infos);
			return 	result;
		}

		read_flag	 = true; // READING - Subscriber���� �����͸� �д� ������ �Ǻ��ϱ� ���ؼ� �߰�.

		int							i		= 0;
		Node<DataSample>* node = data_sample_list.getNodeByIndex(0);


		if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == INSTANCE_PRESENTATION_QOS)
		{
			while(data_sample_list.getSize() > i++)
			{
				node = node->rear;

				if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
						(node->value->info->instance_state & instance_states))
				{
					if(max_sample > data_values->getSize())
					{
						node->value->info->sample_state	= READ_SAMPLE_STATE;
						node->value->info->view_state		= NOT_NEW_VIEW_STATE;

						data_values->insertInRear(node->value->value);
						sample_infos->insertInRear(node->value->info);

						data_sample_list.cutNode(node); // ���� ��� ����
						delete(node->value);
						delete(node);
					}
					else
					{
						break;
					}
				}
			}
		}
		else if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == TOPIC_PRESENTATION_QOS)
		{
			if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.ordered_access == true)
			{
				if(((DataReaderQos*)qos)->destination_order.kind == BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS)
				{
					while(data_sample_list.getSize() > i++)
					{
						node = node->rear;

						if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
								(node->value->info->instance_state & instance_states))
						{
							if(max_sample > data_values->getSize())
							{
								node->value->info->sample_state	= READ_SAMPLE_STATE;
								node->value->info->view_state		= NOT_NEW_VIEW_STATE;

								data_values->insertInRear(node->value->value);
								sample_infos->insertInRear(node->value->info);

								data_sample_list.cutNode(node); // ���� ��� ����
								delete(node->value);
								delete(node);
							}
							else
							{
								break;
							}
						}
					}
				}
				else // Source_timestamp�� ����Ͽ� List�� ����
				{
					Time_t 	cTime;
					int			count;
					int			j;

					// ��������� �ý��� �����ϰ� 0�ʿ� �����͸� ��� ������ 0,0���� �ʱ�ȭ �Ͽ���
					cTime.sec			= 0;
					cTime.nanosec		= 0;
					j						= 0;

					while(max_sample > j++)
					{
						while(data_sample_list.getSize() > i++)
						{
							node = node->rear;

							if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
									(node->value->info->instance_state & instance_states))
							{
								if((cTime.sec == 0) && (cTime.nanosec == 0))
								{
									cTime.sec			= node->value->info->source_timestamp.sec;
									cTime.nanosec		= node->value->info->source_timestamp.nanosec;
									count					= i;
								}
								else if(cTime.sec > node->value->info->source_timestamp.sec)
								{
									cTime.sec			= node->value->info->source_timestamp.sec;
									cTime.nanosec		= node->value->info->source_timestamp.nanosec;
									count					= i;
								}
								else if(cTime.sec == node->value->info->source_timestamp.sec)
								{
									if(cTime.nanosec > node->value->info->source_timestamp.nanosec)
									{
										cTime.sec			= node->value->info->source_timestamp.sec;
										cTime.nanosec		= node->value->info->source_timestamp.nanosec;
										count					= i;
									}
								}
							}
						}

						node = data_sample_list.getNodeByIndex(count);

						node->value->info->sample_state	= READ_SAMPLE_STATE;
						node->value->info->view_state		= NOT_NEW_VIEW_STATE;

						data_values->insertInRear(node->value->value);
						sample_infos->insertInRear(node->value->info);

						data_sample_list.cutNode(node);
						delete(node->value);
						delete(node);

						node = data_sample_list.getNodeByIndex(0);

						i						= 0;
						cTime.sec			= 0;
						cTime.nanosec		= 0;
					}
				}
			}
			else // false
			 {
				while(data_sample_list.getSize() > i++)
				{
					node = node->rear;

					if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
							(node->value->info->instance_state & instance_states))
					{
						if(max_sample > data_values->getSize())
						{
							node->value->info->sample_state	= READ_SAMPLE_STATE;
							node->value->info->view_state		= NOT_NEW_VIEW_STATE;

							data_values->insertInRear(node->value->value);
							sample_infos->insertInRear(node->value->info);

							data_sample_list.cutNode(node);
							delete(node->value);
							delete(node);
						}
						else
						{
							break;
						}
					}
				}
			}
		}
		else if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == GROUP_PRESENTATION_QOS)
		{
			// presentation qos�� group�̸� ordered_access�� true�� ��쿡�� �ִ� 1���� ������ ��ȯ�Ѵ�.
			 if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.ordered_access == true)
			 {
				 if(((DataReaderQos*)qos)->destination_order.kind == BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS)
				 {
					 while(data_sample_list.getSize() > i++)
					 {
						 node = node->rear;

							if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
									(node->value->info->instance_state & instance_states))
						 {
							 node->value->info->sample_state	= READ_SAMPLE_STATE;
							 node->value->info->view_state		= NOT_NEW_VIEW_STATE;

							 data_values->insertInRear(node->value->value);
							 sample_infos->insertInRear(node->value->info);

							 data_sample_list.cutNode(node);
							 delete(node->value);
							 delete(node);

							 break;
						 }
					 }
				 }
				 else // destination_order.kind == SOURCE_TIMESTAMP
				 {
					 Time_t 	cTime;
					 int			count;

					 // ��������� �ý��� �����ϰ� 0�ʿ� �����͸� �� �� �����״� 0,0���� �ʱ�ȭ �Ͽ���
					 cTime.sec			= 0;
					 cTime.nanosec	= 0;

					 // ���� ���� ���� ã��
					 while(data_sample_list.getSize() > i++)
					 {
						 node = node->rear;

							if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
									(node->value->info->instance_state & instance_states))
						 {
							 if((cTime.sec == 0) && (cTime.nanosec == 0))
							 {
								 cTime.sec			= node->value->info->source_timestamp.sec;
								 cTime.nanosec	= node->value->info->source_timestamp.nanosec;
								 count				= i;
							 }
							 else if(cTime.sec > node->value->info->source_timestamp.sec)
							 {
								 cTime.sec			= node->value->info->source_timestamp.sec;
								 cTime.nanosec	= node->value->info->source_timestamp.nanosec;
								 count				= i;
							 }
							 else if(cTime.sec == node->value->info->source_timestamp.sec)
							 {
								 if(cTime.nanosec > node->value->info->source_timestamp.nanosec)
								 {
									 cTime.sec			= node->value->info->source_timestamp.sec;
									 cTime.nanosec	= node->value->info->source_timestamp.nanosec;
									 count				= i;
								 }
							 }
						 }
					 }

					 node = data_sample_list.getNodeByIndex(count);

					 node->value->info->sample_state	= READ_SAMPLE_STATE;
					 node->value->info->view_state		= NOT_NEW_VIEW_STATE;

					 data_values->insertInRear(node->value->value);
					 sample_infos->insertInRear(node->value->info);

					 data_sample_list.cutNode(node);
					 delete(node->value);
					 delete(node);
				 }
			 }
			 else // Ordered_access = false
			 {
				 while(data_sample_list.getSize() > i++)
				 {
					 node = node->rear;

						if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
								(node->value->info->instance_state & instance_states))
					 {
						 if(max_sample > data_values->getSize())
						 {
							 node->value->info->sample_state	= READ_SAMPLE_STATE;
							 node->value->info->view_state		= NOT_NEW_VIEW_STATE;

							 data_values->insertInRear(node->value->value);
							 sample_infos->insertInRear(node->value->info);

							 data_sample_list.cutNode(node);
							 delete(node->value);
							 delete(node);
						 }
						 else
						 {
							 break;
						 }
					 }
				 }
			 }
		}

		read_flag	= false;

		if(data_values->getSize() > 0)
		{
			result		= RETCODE_OK;
		}
		else
		{
			result		 = RETCODE_NO_DATA;
		}

		// rtps ���� �ʿ�
		// ���⼭ sample_info�� �پ���?? rank ���� ��������� ��

	}

	ReturnCode_t		DataReader::take_w_condition(DataList* data_values, SampleInfoList* sample_infos, long max_sample, ReadCondition* a_condition)
	{
		ReturnCode_t	result	= RETCODE_OK;

		if(!is_enabled)
		{
			result		=	RETCODE_NOT_ENABLED;
			return	result;
		}
		// get_datareader�� �̱��� �����̹Ƿ� ������ ���� �ּ�ó�� ��
		/*
		if(a_condition->get_datareader() != this)
		{
			result		= RETCODE_PRECONDITION_NOT_MET;
			return	result;
		}
		*/

		if(max_sample == 0)		// Zero-copy�� ��� return_loan�� ȣ���ؾ� �� , �� �� ���� �ʿ�
		{
			result		= return_loan(data_values, sample_infos);
			return 	result;
		}

		read_flag	 = true; // READING - Subscriber���� �����͸� �д� ������ �Ǻ��ϱ� ���ؼ� �߰�.

		int							i		= 0;
		Node<DataSample>* node = data_sample_list.getNodeByIndex(0);


		if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == INSTANCE_PRESENTATION_QOS)
		{
			while(data_sample_list.getSize() > i++)
			{
				node = node->rear;

				if( (node->value->info->sample_state & a_condition->sample_state_mask) && (node->value->info->view_state & a_condition->view_state_mask) &&
						(node->value->info->instance_state & a_condition->instance_state_mask))
				{
					if(max_sample > data_values->getSize())
					{
						node->value->info->sample_state	= READ_SAMPLE_STATE;
						node->value->info->view_state		= NOT_NEW_VIEW_STATE;

						data_values->insertInRear(node->value->value);
						sample_infos->insertInRear(node->value->info);

						data_sample_list.cutNode(node); // ���� ��� ����
						delete(node->value);
						delete(node);
					}
					else
					{
						break;
					}
				}
			}
		}
		else if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == TOPIC_PRESENTATION_QOS)
		{
			if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.ordered_access == true)
			{
				if(((DataReaderQos*)qos)->destination_order.kind == BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS)
				{
					while(data_sample_list.getSize() > i++)
					{
						node = node->rear;

						if( (node->value->info->sample_state & a_condition->sample_state_mask) && (node->value->info->view_state & a_condition->view_state_mask) &&
								(node->value->info->instance_state & a_condition->instance_state_mask))
						{
							if(max_sample > data_values->getSize())
							{
								node->value->info->sample_state	= READ_SAMPLE_STATE;
								node->value->info->view_state		= NOT_NEW_VIEW_STATE;

								data_values->insertInRear(node->value->value);
								sample_infos->insertInRear(node->value->info);

								data_sample_list.cutNode(node); // ���� ��� ����
								delete(node->value);
								delete(node);
							}
							else
							{
								break;
							}
						}
					}
				}
				else // Source_timestamp�� ����Ͽ� List�� ����
				{
					Time_t 	cTime;
					int			count;
					int			j;

					// ��������� �ý��� �����ϰ� 0�ʿ� �����͸� ��� ������ 0,0���� �ʱ�ȭ �Ͽ���
					cTime.sec			= 0;
					cTime.nanosec		= 0;
					j						= 0;

					while(max_sample > j++)
					{
						while(data_sample_list.getSize() > i++)
						{
							node = node->rear;

							if( (node->value->info->sample_state & a_condition->sample_state_mask) && (node->value->info->view_state & a_condition->view_state_mask) &&
									(node->value->info->instance_state & a_condition->instance_state_mask))
							{
								if((cTime.sec == 0) && (cTime.nanosec == 0))
								{
									cTime.sec			= node->value->info->source_timestamp.sec;
									cTime.nanosec		= node->value->info->source_timestamp.nanosec;
									count					= i;
								}
								else if(cTime.sec > node->value->info->source_timestamp.sec)
								{
									cTime.sec			= node->value->info->source_timestamp.sec;
									cTime.nanosec		= node->value->info->source_timestamp.nanosec;
									count					= i;
								}
								else if(cTime.sec == node->value->info->source_timestamp.sec)
								{
									if(cTime.nanosec > node->value->info->source_timestamp.nanosec)
									{
										cTime.sec			= node->value->info->source_timestamp.sec;
										cTime.nanosec		= node->value->info->source_timestamp.nanosec;
										count					= i;
									}
								}
							}
						}

						node = data_sample_list.getNodeByIndex(count);

						node->value->info->sample_state	= READ_SAMPLE_STATE;
						node->value->info->view_state		= NOT_NEW_VIEW_STATE;

						data_values->insertInRear(node->value->value);
						sample_infos->insertInRear(node->value->info);

						data_sample_list.cutNode(node);
						delete(node->value);
						delete(node);

						node = data_sample_list.getNodeByIndex(0);

						i						= 0;
						cTime.sec 			= 0;
						cTime.nanosec		= 0;

					}
				}
			}
			else // false
			 {
				while(data_sample_list.getSize() > i++)
				{
					node = node->rear;

					if( (node->value->info->sample_state & a_condition->sample_state_mask) && (node->value->info->view_state & a_condition->view_state_mask) &&
							(node->value->info->instance_state & a_condition->instance_state_mask))
					{
						if(max_sample > data_values->getSize())
						{
							node->value->info->sample_state	= READ_SAMPLE_STATE;
							node->value->info->view_state		= NOT_NEW_VIEW_STATE;

							data_values->insertInRear(node->value->value);
							sample_infos->insertInRear(node->value->info);

							data_sample_list.cutNode(node);
							delete(node->value);
							delete(node);
						}
						else
						{
							break;
						}
					}
				}
			}
		}
		else if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == GROUP_PRESENTATION_QOS)
		{
			// presentation qos�� group�̸� ordered_access�� true�� ��쿡�� �ִ� 1���� ������ ��ȯ�Ѵ�.
			 if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.ordered_access == true)
			 {
				 if(((DataReaderQos*)qos)->destination_order.kind == BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS)
				 {
					 while(data_sample_list.getSize() > i++)
					 {
						 node = node->rear;

						 if( (node->value->info->sample_state & a_condition->sample_state_mask) && (node->value->info->view_state & a_condition->view_state_mask) &&
								 (node->value->info->instance_state & a_condition->instance_state_mask))
						 {
							 node->value->info->sample_state	= READ_SAMPLE_STATE;
							 node->value->info->view_state		= NOT_NEW_VIEW_STATE;

							 data_values->insertInRear(node->value->value);
							 sample_infos->insertInRear(node->value->info);

							 data_sample_list.cutNode(node);
							 delete(node->value);
							 delete(node);
						 }
					 }
				 }
				 else // destination_order.kind == SOURCE_TIMESTAMP
				 {
					 Time_t 	cTime;
					 int			count;

					 // ��������� �ý��� �����ϰ� 0�ʿ� �����͸� �� �� �����״� 0,0���� �ʱ�ȭ �Ͽ���
					 cTime.sec			= 0;
					 cTime.nanosec	= 0;

					 // ���� ���� ���� ã��
					 while(data_sample_list.getSize() > i++)
					 {
						 node = node->rear;

						 if( (node->value->info->sample_state & a_condition->sample_state_mask) && (node->value->info->view_state & a_condition->view_state_mask) &&
								 (node->value->info->instance_state & a_condition->instance_state_mask))
						 {
							 if((cTime.sec == 0) && (cTime.nanosec == 0))
							 {
								 cTime.sec			= node->value->info->source_timestamp.sec;
								 cTime.nanosec	= node->value->info->source_timestamp.nanosec;
								 count				= i;
							 }
							 else if(cTime.sec > node->value->info->source_timestamp.sec)
							 {
								 cTime.sec			= node->value->info->source_timestamp.sec;
								 cTime.nanosec	= node->value->info->source_timestamp.nanosec;
								 count				= i;
							 }
							 else if(cTime.sec == node->value->info->source_timestamp.sec)
							 {
								 if(cTime.nanosec > node->value->info->source_timestamp.nanosec)
								 {
									 cTime.sec			= node->value->info->source_timestamp.sec;
									 cTime.nanosec	= node->value->info->source_timestamp.nanosec;
									 count				= i;
								 }
							 }
						 }
					 }

					 node = data_sample_list.getNodeByIndex(count);

					 node->value->info->sample_state	= READ_SAMPLE_STATE;
					 node->value->info->view_state		= NOT_NEW_VIEW_STATE;

					 data_values->insertInRear(node->value->value);
					 sample_infos->insertInRear(node->value->info);

					 data_sample_list.cutNode(node);
					 delete(node->value);
					 delete(node);
				 }
			 }
			 else // Ordered_access = false
			 {
				 while(data_sample_list.getSize() > i++)
				 {
					 node = node->rear;

					 if( (node->value->info->sample_state & a_condition->sample_state_mask) && (node->value->info->view_state & a_condition->view_state_mask) &&
							 (node->value->info->instance_state & a_condition->instance_state_mask))
					 {
						 if(max_sample > data_values->getSize())
						 {
							 node->value->info->sample_state	= READ_SAMPLE_STATE;
							 node->value->info->view_state		= NOT_NEW_VIEW_STATE;

							 data_values->insertInRear(node->value->value);
							 sample_infos->insertInRear(node->value->info);

							 data_sample_list.cutNode(node);
							 delete(node->value);
							 delete(node);
						 }
						 else
						 {
							 break;
						 }
					 }
				 }
			 }
		}

		read_flag	= false;

		if(data_values->getSize() > 0)
		{
			result		= RETCODE_OK;
		}
		else
		{
			result		 = RETCODE_NO_DATA;
		}

		// rtps ���� �ʿ�
		// ���⼭ sample_info�� �پ���?? rank ���� ��������� ��
	}

	// ���� �������� ���� DataReader�� Data�� �����Ͽ� ������, SampleInfo ���� �����ؾ� ��
	// read Operation�� ��������� max_samples, Sample_state�� view_state, instance_state�� ������ ���� �ٸ� ��.
	// max_sample = 1, sample_state = NOT_READ, view_state = ANY_VIEW, instance_state = ANY_INSTANACE
	// read_next_sample�� �����ϸ� �ٸ� ���� ���� ���� ��带 �����Ѵٴ� ��
	ReturnCode_t		DataReader::take_next_sample(DDS_Data* data_value, SampleInfo* sample_info)
	{
		ReturnCode_t	result	= RETCODE_OK;

		if(!is_enabled)
		{
			result		=	RETCODE_NOT_ENABLED;
			return	result;
		}

		read_flag	 = true; // READING - Subscriber���� �����͸� �д� ������ �Ǻ��ϱ� ���ؼ� �߰�.

		int							i		= 0;
		Node<DataSample>* node = data_sample_list.getNodeByIndex(0);


		if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == INSTANCE_PRESENTATION_QOS)
		{
			while(data_sample_list.getSize() > i++)
			{
				node = node->rear;

				if( (node->value->info->sample_state & NOT_READ_SAMPLE_STATE) && (node->value->info->view_state & ANY_VIEW_STATE) &&
						(node->value->info->instance_state & ANY_INSTANCE_STATE))
				{
						node->value->info->sample_state	= READ_SAMPLE_STATE;
						node->value->info->view_state		= NOT_NEW_VIEW_STATE;

						data_value= node->value->value;
						sample_info = node->value->info;

						data_sample_list.cutNode(node); // ���� ��� ����
						delete(node->value);
						delete(node);
				}
			}
		}
		else if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == TOPIC_PRESENTATION_QOS)
		{
			if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.ordered_access == true)
			{
				if(((DataReaderQos*)qos)->destination_order.kind == BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS)
				{
					while(data_sample_list.getSize() > i++)
					{
						node = node->rear;

						if( (node->value->info->sample_state & NOT_READ_SAMPLE_STATE) && (node->value->info->view_state & ANY_VIEW_STATE) &&
								(node->value->info->instance_state & ANY_INSTANCE_STATE))
						{
							node->value->info->sample_state	= READ_SAMPLE_STATE;
							node->value->info->view_state		= NOT_NEW_VIEW_STATE;

							data_value= node->value->value;
							sample_info = node->value->info;

							data_sample_list.cutNode(node); // ���� ��� ����
							delete(node->value);
							delete(node);
						}
					}
				}
				else // Source_timestamp�� ����Ͽ� List�� ����
				{
					Time_t 	cTime;
					int			count;

					// ��������� �ý��� �����ϰ� 0�ʿ� �����͸� ��� ������ 0,0���� �ʱ�ȭ �Ͽ���
					cTime.sec			= 0;
					cTime.nanosec		= 0;

					while(data_sample_list.getSize() > i++)
					{
						node = node->rear;

						if( (node->value->info->sample_state & NOT_READ_SAMPLE_STATE) && (node->value->info->view_state & ANY_VIEW_STATE) &&
								(node->value->info->instance_state & ANY_INSTANCE_STATE))
						{
							if((cTime.sec == 0) && (cTime.nanosec == 0))
							{
								cTime.sec			= node->value->info->source_timestamp.sec;
								cTime.nanosec		= node->value->info->source_timestamp.nanosec;
								count					= i;
							}
							else if(cTime.sec > node->value->info->source_timestamp.sec)
							{
								cTime.sec			= node->value->info->source_timestamp.sec;
								cTime.nanosec		= node->value->info->source_timestamp.nanosec;
								count					= i;
							}
							else if(cTime.sec == node->value->info->source_timestamp.sec)
							{
								if(cTime.nanosec > node->value->info->source_timestamp.nanosec)
								{
									cTime.sec			= node->value->info->source_timestamp.sec;
									cTime.nanosec		= node->value->info->source_timestamp.nanosec;
									count					= i;
								}
							}
						}
					}

					node = data_sample_list.getNodeByIndex(count);

					node->value->info->sample_state	= READ_SAMPLE_STATE;
					node->value->info->view_state		= NOT_NEW_VIEW_STATE;

					data_value= node->value->value;
					sample_info = node->value->info;

					data_sample_list.cutNode(node); // ���� ��� ����
					delete(node->value);
					delete(node);
				}
			}
			else // false
			 {
				while(data_sample_list.getSize() > i++)
				{
					node = node->rear;

					if( (node->value->info->sample_state & NOT_READ_SAMPLE_STATE) && (node->value->info->view_state & ANY_VIEW_STATE) &&
							(node->value->info->instance_state & ANY_INSTANCE_STATE))
					{
						node->value->info->sample_state	= READ_SAMPLE_STATE;
						node->value->info->view_state		= NOT_NEW_VIEW_STATE;

						data_value= node->value->value;
						sample_info = node->value->info;

						data_sample_list.cutNode(node); // ���� ��� ����
						delete(node->value);
						delete(node);
					}
				}
			}
		}
		else if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == GROUP_PRESENTATION_QOS)
		{
			// presentation qos�� group�̸� ordered_access�� true�� ��쿡�� �ִ� 1���� ������ ��ȯ�Ѵ�.
			 if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.ordered_access == true)
			 {
				 if(((DataReaderQos*)qos)->destination_order.kind == BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS)
				 {
					 while(data_sample_list.getSize() > i++)
					 {
						 node = node->rear;

						 if( (node->value->info->sample_state & NOT_READ_SAMPLE_STATE) && (node->value->info->view_state & ANY_VIEW_STATE) &&
								 (node->value->info->instance_state & ANY_INSTANCE_STATE))
						 {
							 node->value->info->sample_state	= READ_SAMPLE_STATE;
							 node->value->info->view_state		= NOT_NEW_VIEW_STATE;

							 data_value= node->value->value;
							 sample_info = node->value->info;

							 data_sample_list.cutNode(node); // ���� ��� ����
							 delete(node->value);
							 delete(node);
						 }
					 }
				 }
				 else // destination_order.kind == SOURCE_TIMESTAMP
				 {
					 Time_t 	cTime;
					 int			count;

					 // ��������� �ý��� �����ϰ� 0�ʿ� �����͸� �� �� �����״� 0,0���� �ʱ�ȭ �Ͽ���
					 cTime.sec			= 0;
					 cTime.nanosec	= 0;

					 // ���� ���� ���� ã��
					 while(data_sample_list.getSize() > i++)
					 {
						 node = node->rear;

						 if( (node->value->info->sample_state & NOT_READ_SAMPLE_STATE) && (node->value->info->view_state & ANY_VIEW_STATE) &&
								 (node->value->info->instance_state & ANY_INSTANCE_STATE))
						 {
							 if((cTime.sec == 0) && (cTime.nanosec == 0))
							 {
								 cTime.sec			= node->value->info->source_timestamp.sec;
								 cTime.nanosec	= node->value->info->source_timestamp.nanosec;
								 count				= i;
							 }
							 else if(cTime.sec > node->value->info->source_timestamp.sec)
							 {
								 cTime.sec			= node->value->info->source_timestamp.sec;
								 cTime.nanosec	= node->value->info->source_timestamp.nanosec;
								 count				= i;
							 }
							 else if(cTime.sec == node->value->info->source_timestamp.sec)
							 {
								 if(cTime.nanosec > node->value->info->source_timestamp.nanosec)
								 {
									 cTime.sec			= node->value->info->source_timestamp.sec;
									 cTime.nanosec	= node->value->info->source_timestamp.nanosec;
									 count				= i;
								 }
							 }
						 }
					 }

					 node = data_sample_list.getNodeByIndex(count);

					 node->value->info->sample_state	= READ_SAMPLE_STATE;
					 node->value->info->view_state		= NOT_NEW_VIEW_STATE;

					 data_value= node->value->value;
					 sample_info = node->value->info;

					 data_sample_list.cutNode(node);
					 delete(node->value);
					 delete(node);
				 }
			 }
			 else // Ordered_access = false
			 {
				 while(data_sample_list.getSize() > i++)
				 {
					 node = node->rear;

					 if( (node->value->info->sample_state & NOT_READ_SAMPLE_STATE) && (node->value->info->view_state & ANY_VIEW_STATE) &&
							 (node->value->info->instance_state & ANY_INSTANCE_STATE))
					 {
						 node->value->info->sample_state	= READ_SAMPLE_STATE;
						 node->value->info->view_state		= NOT_NEW_VIEW_STATE;

						 data_value= node->value->value;
						 sample_info = node->value->info;

						 data_sample_list.cutNode(node); // ���� ��� ����
						 delete(node->value);
						 delete(node);
					 }
				 }
			 }
		}

		read_flag	= false;

		if(data_value != NULL)
		{
			result		= RETCODE_OK;
		}
		else
		{
			result		 = RETCODE_NO_DATA;
		}

		// rtps ���� �ʿ�
		// ���⼭ sample_info�� �پ���?? rank ���� ��������� ��
	}

	// read �Լ��� �����ϸ� instance�� Ȯ���ϴ� ���� �ٸ���.
	ReturnCode_t		DataReader::read_instance(DataList* data_values, SampleInfoList* sample_infos, long max_sample, InstanceHandle_t a_handle, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states)
	{
		ReturnCode_t	result	= RETCODE_OK;

		bool	Invalid_Flag = false;

		if(!is_enabled)
		{
			result		=	RETCODE_NOT_ENABLED;
			return	result;
		}

		if(max_sample == 0)		// Zero-copy�� ��� return_loan�� ȣ���ؾ� �� , �� �� ���� �ʿ�
		{
			result		= return_loan(data_values, sample_infos);
			return 	result;
		}

		read_flag	 = true; // READING - Subscriber���� �����͸� �д� ������ �Ǻ��ϱ� ���ؼ� �߰�.

		int							i		= 0;
		Node<DataSample>* node = data_sample_list.getNodeByIndex(0);


		if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == INSTANCE_PRESENTATION_QOS)
		{
			while(data_sample_list.getSize() > i++)
			{
				node = node->rear;

				// �Ѿ���� �Ķ���� 3���� ������ ��� �����ϴ� Data, SampleInfo�� ��� �����͸� ����
				if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
						(node->value->info->instance_state & instance_states) && (node->value->info->instance_handle == a_handle))
				{
					Invalid_Flag = true;

					if(max_sample > data_values->getSize()) // �ִ� �����ŭ �����͸� ����
					{
						node->value->info->sample_state	= READ_SAMPLE_STATE;
						node->value->info->view_state		= NOT_NEW_VIEW_STATE;

						data_values->insertInRear(node->value->value);
						sample_infos->insertInRear(node->value->info);
					}
					else // ���� ��ŭ ��� �о����� while���� Ż��... ���� Ż�� ���� RTPS�� �̿��ؼ� ������ �κа� return ó���� ��
					{
						break;
					}
				}
			}
		}
		else if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == TOPIC_PRESENTATION_QOS)
		{
			if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.ordered_access == true)
			{
				if(((DataReaderQos*)qos)->destination_order.kind == BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS)
				{
					while(data_sample_list.getSize() > i++)
					{
						node = node->rear;

						if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
								(node->value->info->instance_state & instance_states) && (node->value->info->instance_handle == a_handle))
						{
							Invalid_Flag = true;

							if(max_sample > data_values->getSize())
							{
								node->value->info->sample_state	= READ_SAMPLE_STATE;
								node->value->info->view_state		= NOT_NEW_VIEW_STATE;

								data_values->insertInRear(node->value->value);
								sample_infos->insertInRear(node->value->info);
							}
							else
							{
								break;
							}
						}
					}
				}
				else // Source_timestamp�� ����Ͽ� List�� ����
				{
					Time_t 	cTime; // Source_timestamp���� ���ϱ� ���� ����
					bool 		cFlag; // �о��� ���������� Ȯ��
					//int			count[max_sample]; // �о�� �� �����͵��� ��� Index�� ����
					int*		count	= new int[max_sample];
					int	 		j ;

					// ��������� �ý��� �����ϰ� 0�ʿ� �����͸� ��� ������ 0,0���� �ʱ�ȭ �Ͽ���
					cTime.sec			= 0;
					cTime.nanosec		= 0;

					// ���� ���� Source_Timestamp ���� ã��
					while(data_sample_list.getSize() > i++)
					{
						node = node->rear;

						if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
								(node->value->info->instance_state & instance_states) && (node->value->info->instance_handle == a_handle))
						{
							Invalid_Flag = true;

							if((cTime.sec == 0) && (cTime.nanosec == 0)) // �ʱⰪ ����
							{
								cTime.sec			= node->value->info->source_timestamp.sec;
								cTime.nanosec		= node->value->info->source_timestamp.nanosec;
								count[0]				= i;
							}
							else if(cTime.sec > node->value->info->source_timestamp.sec)
							{
								cTime.sec			= node->value->info->source_timestamp.sec;
								cTime.nanosec		= node->value->info->source_timestamp.nanosec;
								count[0]				= i;
							}
							else if(cTime.sec == node->value->info->source_timestamp.sec)
							{
								if(cTime.nanosec > node->value->info->source_timestamp.nanosec)
								{
									cTime.sec			= node->value->info->source_timestamp.sec;
									cTime.nanosec		= node->value->info->source_timestamp.nanosec;
									count[0]				= i;
								}
							}
						}
					}

					i		= 0;
					j		= 0;
					cFlag	= true;
					cTime.sec			= 0;
					cTime.nanosec		= 0;

					while(max_sample-1 > j++)
					{
						node = data_sample_list.getNodeByIndex(0); // ��� ��带 �˻��ϱ� ���ؼ� ����� ó������ ����

						while(data_sample_list.getSize() > i++)
						{
							node = node->rear;

							if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
									(node->value->info->instance_state & instance_states) && (node->value->info->instance_handle == a_handle))
							{
								int		k;
								for(k = 0; k < j; k++) // ������ ��ϵǾ� �ִ� ���̸� ���� �ʿ䰡 �����Ƿ� üũ
								{
									if(count[k] == i) // ������ ��� �Ǿ����� Flag���� ����
									{
										cFlag	= false;
										break;
									}
								}

								if((cTime.sec == 0) && (cTime.nanosec == 0) && cFlag)
								{
									cTime.sec			= node->value->info->source_timestamp.sec;
									cTime.nanosec		= node->value->info->source_timestamp.nanosec;
									count[j]				= i;
								}

								if(cTime.sec > node->value->info->source_timestamp.sec && cFlag)
								{
									cTime.sec 			= node->value->info->source_timestamp.sec;
									cTime.nanosec		= node->value->info->source_timestamp.nanosec;
									count[j]				= i;
								}
								else if(cTime.sec == node->value->info->source_timestamp.sec && cFlag)
								{
									if(cTime.nanosec > node->value->info->source_timestamp.nanosec)
									{
										cTime.sec 			= node->value->info->source_timestamp.sec;
										cTime.nanosec		= node->value->info->source_timestamp.nanosec;
										count[j]				= i;
									}
								}
							}

							cFlag = true;
						}
					}

					for(i = 0; i < max_sample; i++) // ������ ������ ����� Index�� �̿��ؼ� �����͸� ����
					{
						node = data_sample_list.getNodeByIndex(count[i]);

						node->value->info->sample_state	= READ_SAMPLE_STATE;
						node->value->info->view_state		= NOT_NEW_VIEW_STATE;

						data_values->insertInRear(node->value->value);
						sample_infos->insertInRear(node->value->info);
					}
				}
			}
			else // Ordered_access = false
			 {
				while(data_sample_list.getSize() > i++)
				{
					node = node->rear;

					if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
							(node->value->info->instance_state & instance_states) && (node->value->info->instance_handle == a_handle))
					{
						Invalid_Flag = true;

						if(max_sample > data_values->getSize())
						{
							node->value->info->sample_state	= READ_SAMPLE_STATE;
							node->value->info->view_state		= NOT_NEW_VIEW_STATE;

							data_values->insertInRear(node->value->value);
							sample_infos->insertInRear(node->value->info);
						}
						else
						{
							break;
						}
					}
				}
			}
		}
		else if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == GROUP_PRESENTATION_QOS)
		{
			// presentation qos�� group�̸� ordered_access�� true�� ��쿡�� �ִ� 1���� ������ ��ȯ�Ѵ�.
			if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.ordered_access == true)
			{
				if(((DataReaderQos*)qos)->destination_order.kind == BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS)
				{
					while(data_sample_list.getSize() > i++)
					{
						node = node->rear;

						if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
								(node->value->info->instance_state & instance_states) && (node->value->info->instance_handle == a_handle))
						{
							Invalid_Flag = true;

							node->value->info->sample_state	= READ_SAMPLE_STATE;
							node->value->info->view_state		= NOT_NEW_VIEW_STATE;

							data_values->insertInRear(node->value->value);
							sample_infos->insertInRear(node->value->info);

							break;
						}
					 }
				 }
				 else // destination_order.kind == SOURCE_TIMESTAMP
				 {
					 Time_t 	cTime;
					 int		count; // Data, SampleInfo �ϳ��� ������ �ǹǷ� �迭�� �̿����� ����

					 // ��������� �ý��� �����ϰ� 0�ʿ� �����͸� �� �� �����״� 0,0���� �ʱ�ȭ �Ͽ���
					 cTime.sec			= 0;
					 cTime.nanosec	= 0;

					 // ���� ���� ���� ã��
					 while(data_sample_list.getSize() > i++)
					 {
						 node = node->rear;

						 if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
								 (node->value->info->instance_state & instance_states) && (node->value->info->instance_handle == a_handle))
						 {
							 Invalid_Flag = true;

							 if((cTime.sec == 0) && (cTime.nanosec == 0))
							 {
								 cTime.sec			= node->value->info->source_timestamp.sec;
								 cTime.nanosec	= node->value->info->source_timestamp.nanosec;
								 count				= i;
							 }
							 else if(cTime.sec > node->value->info->source_timestamp.sec)
							 {
								 cTime.sec			= node->value->info->source_timestamp.sec;
								 cTime.nanosec	= node->value->info->source_timestamp.nanosec;
								 count				= i;
							 }
							 else if(cTime.sec == node->value->info->source_timestamp.sec)
							 {
								 if(cTime.nanosec > node->value->info->source_timestamp.nanosec)
								 {
									 cTime.sec			= node->value->info->source_timestamp.sec;
									 cTime.nanosec	= node->value->info->source_timestamp.nanosec;
									 count				= i;
								 }
							 }
						 }
					 }

					 node = data_sample_list.getNodeByIndex(count);

					 node->value->info->sample_state	= READ_SAMPLE_STATE;
					 node->value->info->view_state		= NOT_NEW_VIEW_STATE;

					 data_values->insertInRear(node->value->value);
					 sample_infos->insertInRear(node->value->info);
				 }
			}
			else // Ordered_access = false
			{
				while(data_sample_list.getSize() > i++)
				{
					node = node->rear;

					if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
							(node->value->info->instance_state & instance_states) && (node->value->info->instance_handle == a_handle))
					{
						Invalid_Flag = true;

						if(max_sample > data_values->getSize())
						{
							node->value->info->sample_state	= READ_SAMPLE_STATE;
							node->value->info->view_state		= NOT_NEW_VIEW_STATE;

							data_values->insertInRear(node->value->value);
							sample_infos->insertInRear(node->value->info);
						 }
						 else
						 {
							 break;
						 }
					 }
				 }
			 }
		}

		read_flag	= false;

		// �����͸� ������ ����Ʈ�� ũ�Ⱑ 0���� ũ�ٸ� �����͸� ���� ����.. ������ ���ʿ� �Ѿ���� ����Ʈ�� ũ�Ⱑ 0�̶�� ���� ������ �� �����Ƿ� ����� �ʿ���
		// �����ϰ� �ϸ� ���ʿ� ����Ʈ ũ�⸦ �����Ͽ��ٰ� ���ϰų� �����͸� �д� ���� result = RETCODE_OK�� �ִ� ����� �ְ���..
		if(Invalid_Flag == false)
		{
			result		= RETCODE_BAD_PARAMETER;
		}

		if(data_values->getSize() > 0)
		{
			result		= RETCODE_OK;
		}
		else // �����͸� ã�� ��������(�ƿ� �����͸� �ϳ��� ���� ���� ���)
		{
			result		 = RETCODE_NO_DATA;
		}

		return	result;
		// rtps ���� �ʿ�
		// ���⼭ sample_info�� �پ���?? rank ���� ��������� ��
	}

	// Ordered_access�� ���ؼ� Handle ���� ������ ��� ���ؼ� Timestamp ����ϴ� ������ ������
	ReturnCode_t		DataReader::read_next_instance(DataList* data_values, SampleInfoList* sample_infos, long max_sample, InstanceHandle_t previous_handle, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states)
	{
		ReturnCode_t	result	= RETCODE_OK;

		if(!is_enabled)
		{
			result		=	RETCODE_NOT_ENABLED;
			return	result;
		}

		if(max_sample == 0)		// Zero-copy�� ��� return_loan�� ȣ���ؾ� �� , �� �� ���� �ʿ�
		{
			result		= return_loan(data_values, sample_infos);
			return 	result;
		}

		read_flag = true;

		int							i		= 0;
		Node<DataSample>* node	= data_sample_list.getNodeByIndex(0);

		InstanceHandle_t	smallest_handle = 0;


		// previous_handle���ٴ� ũ�� ���� ���� Handle�� ã��
		while(data_sample_list.getSize() > i++)
		{
			node	= node->rear;

			if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
					(node->value->info->instance_state & instance_states) && (node->value->info->instance_handle > previous_handle))
			{
				if(smallest_handle == 0)
				{
					smallest_handle = node->value->info->instance_handle;
				}
				else if(smallest_handle > node->value->info->instance_handle)
				{
					smallest_handle = node->value->info->instance_handle;
				}
			}
		}

		node	= data_sample_list.getNodeByIndex(0);
		i		= 0;

		if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == INSTANCE_PRESENTATION_QOS)
		{
			while(data_sample_list.getSize() > i++)
			{
				node = node->rear;

				// �Ѿ���� �Ķ���� 3���� ������ ��� �����ϴ� Data, SampleInfo�� ��� �����͸� ����
				if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
						(node->value->info->instance_state & instance_states) && (node->value->info->instance_handle == smallest_handle))
				{
					if(max_sample > data_values->getSize()) // �ִ� �����ŭ �����͸� ����
					{
						node->value->info->sample_state	= READ_SAMPLE_STATE;
						node->value->info->view_state		= NOT_NEW_VIEW_STATE;

						data_values->insertInRear(node->value->value);
						sample_infos->insertInRear(node->value->info);
					}
					else // ���� ��ŭ ��� �о����� while���� Ż��... ���� Ż�� ���� RTPS�� �̿��ؼ� ������ �κа� return ó���� ��
					{
						break;
					}
				}
			}
		}
		else if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == TOPIC_PRESENTATION_QOS)
		{
			if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.ordered_access == true)
			{
				if(((DataReaderQos*)qos)->destination_order.kind == BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS)
				{
					while(data_sample_list.getSize() > i++)
					{
						node = node->rear;

						if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
								(node->value->info->instance_state & instance_states) && (node->value->info->instance_handle == smallest_handle))
						{
							if(max_sample > data_values->getSize())
							{
								node->value->info->sample_state	= READ_SAMPLE_STATE;
								node->value->info->view_state		= NOT_NEW_VIEW_STATE;

								data_values->insertInRear(node->value->value);
								sample_infos->insertInRear(node->value->info);
							}
							else
							{
								break;
							}
						}
					}
				}
				else // Source_timestamp�� ����Ͽ� List�� ����
				{
					Time_t	cTime;
					bool		cFlag;
					int			index;
					int			max_len;
					int			j;

					cTime.sec			= 0;
					cTime.nanosec		= 0;

					// �ּҰ��� ã��
					while(data_sample_list.getSize() > i++)
					{
						node = node->rear;

						// State�� �����ϴ� ������ �� TimeStamp�� ����ؾ� ��
						if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
								(node->value->info->instance_state & instance_states) && (node->value->info->instance_handle == smallest_handle))
						{
							if((cTime.sec == 0) && (cTime.nanosec == 0))
							{
								cTime.sec			= node->value->info->source_timestamp.sec;
								cTime.nanosec		= node->value->info->source_timestamp.nanosec;

								index	= i;
								max_len++;
							}
							else if(cTime.sec > node->value->info->source_timestamp.sec)
							{
								cTime.sec			= node->value->info->source_timestamp.sec;
								cTime.nanosec		= node->value->info->source_timestamp.nanosec;

								index	= i;
								max_len++;
							}
							else if( (cTime.sec == node->value->info->source_timestamp.sec) && (cTime.nanosec > node->value->info->source_timestamp.nanosec))
							{
								cTime.sec			= node->value->info->source_timestamp.sec;
								cTime.nanosec		= node->value->info->source_timestamp.nanosec;

								index	= i;
								max_len++;
							}
						}
					}

					// ���� ���� �����Ͱ� ����ִ� Index�� ��带 �̵����Ѽ� �ش� ����� �����͸� ����
					node	=	data_sample_list.getNodeByIndex(index);

					node->value->info->sample_state	= READ_SAMPLE_STATE;
					node->value->info->view_state		= NOT_NEW_VIEW_STATE;

					data_values->insertInRear(node->value->value);
					sample_infos->insertInRear(node->value->info);

					// ������ ���� �� �ִ� �������� ũ�⺸�� ���ѵ� ũ�⺸�� ũ�ٸ�
					// �ִ� ũ��� 5�ε� ���� �� �ִ� �����Ͱ� �̺��� �۴ٸ� �ִ� ũ�⸸ŭ �����͸� ���� �� �����Ƿ� �ǹ̾��� ���� ������ �ʾƵ� ��

					if(max_sample <= max_len)
					{
						max_len = max_sample;
					}

					//int		dup_data[max_len];
					int*	dup_data	= new int[max_len];
					int		k;
					dup_data[0] = index;

					cFlag 	= true;
					i			= 0;
					j			= 0;

					cTime.sec			= 0;
					cTime.nanosec		= 0;

					// ���� ���� �� ���� ũ���� ������ �߿��� ���� ���� ������ ���ʷ� List�� ����
					while(max_len-1 > j++)
					{
						node		=  data_sample_list.getNodeByIndex(0);
						cFlag		=  true;

						while(data_sample_list.getSize() > i++)
						{
							node = node->rear;
							if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
									(node->value->info->instance_state & instance_states) && (node->value->info->instance_handle == smallest_handle))
							{
								for(k = 0; k < j; k++) // ������ �����ߴ� Index�� ��ģ�ٸ� �ߺ��Ǵ� ���̹Ƿ� üũ, Flag���� True��� ��ġ�� �ʴ� �����Ͷ�� �ǹ�
								{
									if(dup_data[k] == i)
									{
										cFlag = false;
										break;
									}
								}

								if((cTime.sec == 0) && (cTime.nanosec == 0) && cFlag)
								{
									cTime.sec			= node->value->info->source_timestamp.sec;
									cTime.nanosec		= node->value->info->source_timestamp.nanosec;

									dup_data[j] = i;
								}
								else if(cTime.sec > node->value->info->source_timestamp.sec && cFlag)
								{
									cTime.sec			= node->value->info->source_timestamp.sec;
									cTime.nanosec		= node->value->info->source_timestamp.nanosec;

									dup_data[j] = i;
								}
								else if( (cTime.sec == node->value->info->source_timestamp.sec) && (cTime.nanosec > node->value->info->source_timestamp.nanosec) && cFlag)
								{
									cTime.sec			= node->value->info->source_timestamp.sec;
									cTime.nanosec		= node->value->info->source_timestamp.nanosec;

									dup_data[j] = i;
								}
							}
						}

						node	=	data_sample_list.getNodeByIndex(dup_data[j]);

						node->value->info->sample_state	= READ_SAMPLE_STATE;
						node->value->info->view_state		= NOT_NEW_VIEW_STATE;

						data_values->insertInRear(node->value->value);
						sample_infos->insertInRear(node->value->info);
					}
				}
			}
			else // Ordered_access = false
			 {
				while(data_sample_list.getSize() > i++)
				{
					node = node->rear;

					if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
							(node->value->info->instance_state & instance_states) && (node->value->info->instance_handle == smallest_handle))
					{
						if(max_sample > data_values->getSize())
						{
							node->value->info->sample_state	= READ_SAMPLE_STATE;
							node->value->info->view_state		= NOT_NEW_VIEW_STATE;

							data_values->insertInRear(node->value->value);
							sample_infos->insertInRear(node->value->info);
						}
						else
						{
							break;
						}
					}
				}
			}
		}
		else if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == GROUP_PRESENTATION_QOS)
		{
			// presentation qos�� group�̸� ordered_access�� true�� ��쿡�� �ִ� 1���� ������ ��ȯ�Ѵ�.
			 if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.ordered_access == true)
			 {
				 if(((DataReaderQos*)qos)->destination_order.kind == BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS)
				 {
					 while(data_sample_list.getSize() > i++)
					 {
						 node = node->rear;

							if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
									(node->value->info->instance_state & instance_states) && (node->value->info->instance_handle == smallest_handle))
						 {
							 node->value->info->sample_state	= READ_SAMPLE_STATE;
							 node->value->info->view_state		= NOT_NEW_VIEW_STATE;

							 data_values->insertInRear(node->value->value);
							 sample_infos->insertInRear(node->value->info);

							 break;
						 }
					 }
				 }
				 else // destination_order.kind == SOURCE_TIMESTAMP
				 {
					 Time_t 	cTime;
					 int		count; // Data, SampleInfo �ϳ��� ������ �ǹǷ� �迭�� �̿����� ����

					 // ��������� �ý��� �����ϰ� 0�ʿ� �����͸� �� �� �����״� 0,0���� �ʱ�ȭ �Ͽ���
					 cTime.sec			= 0;
					 cTime.nanosec	= 0;

					 // ���� ���� ���� ã��
					 while(data_sample_list.getSize() > i++)
					 {
						 node = node->rear;

						 if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
								 (node->value->info->instance_state & instance_states) && (node->value->info->instance_handle == smallest_handle))
						 {
							 if((cTime.sec == 0) && (cTime.nanosec == 0))
							 {
								 cTime.sec			= node->value->info->source_timestamp.sec;
								 cTime.nanosec	= node->value->info->source_timestamp.nanosec;
								 count				= i;
							 }
							 else if(cTime.sec > node->value->info->source_timestamp.sec)
							 {
								 cTime.sec			= node->value->info->source_timestamp.sec;
								 cTime.nanosec	= node->value->info->source_timestamp.nanosec;
								 count				= i;
							 }
							 else if(cTime.sec == node->value->info->source_timestamp.sec)
							 {
								 if(cTime.nanosec > node->value->info->source_timestamp.nanosec)
								 {
									 cTime.sec			= node->value->info->source_timestamp.sec;
									 cTime.nanosec	= node->value->info->source_timestamp.nanosec;
									 count				= i;
								 }
							 }
						 }
					 }

					 node = data_sample_list.getNodeByIndex(count);

					 node->value->info->sample_state	= READ_SAMPLE_STATE;
					 node->value->info->view_state		= NOT_NEW_VIEW_STATE;

					 data_values->insertInRear(node->value->value);
					 sample_infos->insertInRear(node->value->info);
				 }
			 }
			 else // Ordered_access = false
			 {
				 while(data_sample_list.getSize() > i++)
				 {
					 node = node->rear;

					 if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
							 (node->value->info->instance_state & instance_states) && (node->value->info->instance_handle == smallest_handle))
					 {
						 if(max_sample > data_values->getSize())
						 {
							 node->value->info->sample_state	= READ_SAMPLE_STATE;
							 node->value->info->view_state		= NOT_NEW_VIEW_STATE;

							 data_values->insertInRear(node->value->value);
							 sample_infos->insertInRear(node->value->info);
						 }
						 else
						 {
							 break;
						 }
					 }
				 }
			 }
		}

		read_flag	= false;

		// �����͸� ������ ����Ʈ�� ũ�Ⱑ 0���� ũ�ٸ� �����͸� ���� ����.. ������ ���ʿ� �Ѿ���� ����Ʈ�� ũ�Ⱑ 0�̶�� ���� ������ �� �����Ƿ� ����� �ʿ���
		// �����ϰ� �ϸ� ���ʿ� ����Ʈ ũ�⸦ �����Ͽ��ٰ� ���ϰų� �����͸� �д� ���� result = RETCODE_OK�� �ִ� ����� �ְ���..
		if(data_values->getSize() > 0)
		{
			result		= RETCODE_OK;
		}
		else // �����͸� ã�� ��������(�ƿ� �����͸� �ϳ��� ���� ���� ���)
		{
			result		 = RETCODE_NO_DATA;
		}

		// rtps ���� �ʿ�
		// ���⼭ sample_info�� �پ���?? rank ���� ��������� ��
	}

	ReturnCode_t		DataReader::read_next_instance_w_condition(DataList* data_values, SampleInfoList* sample_infos, long max_sample, InstanceHandle_t previous_handle, ReadCondition* a_condition)
	{
		ReturnCode_t	result	= RETCODE_OK;

		if(!is_enabled)
		{
			result		=	RETCODE_NOT_ENABLED;
			return	result;
		}

		if(max_sample == 0)		// Zero-copy�� ��� return_loan�� ȣ���ؾ� �� , �� �� ���� �ʿ�
		{
			result		= return_loan(data_values, sample_infos);
			return 	result;
		}

		read_flag = true;

		int							i		= 0;
		Node<DataSample>* node	= data_sample_list.getNodeByIndex(0);

		InstanceHandle_t	smallest_handle = 0;


		// previous_handle���ٴ� ũ�� ���� ���� Handle�� ã��
		while(data_sample_list.getSize() > i++)
		{
			node	= node->rear;

			if( (node->value->info->sample_state & a_condition->sample_state_mask) && (node->value->info->view_state & a_condition->view_state_mask) &&
					(node->value->info->instance_state & a_condition->instance_state_mask) && (node->value->info->instance_handle > previous_handle))
			{
				if(smallest_handle == 0)
				{
					smallest_handle = node->value->info->instance_handle;
				}
				else if(smallest_handle > node->value->info->instance_handle)
				{
					smallest_handle = node->value->info->instance_handle;
				}
			}
		}

		node	= data_sample_list.getNodeByIndex(0);
		i		= 0;

		if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == INSTANCE_PRESENTATION_QOS)
		{
			while(data_sample_list.getSize() > i++)
			{
				node = node->rear;

				// �Ѿ���� �Ķ���� 3���� ������ ��� �����ϴ� Data, SampleInfo�� ��� �����͸� ����
				if( (node->value->info->sample_state & a_condition->sample_state_mask) && (node->value->info->view_state & a_condition->view_state_mask) &&
						(node->value->info->instance_state & a_condition->instance_state_mask) && (node->value->info->instance_handle > previous_handle))
				{
					if(max_sample > data_values->getSize()) // �ִ� �����ŭ �����͸� ����
					{
						node->value->info->sample_state	= READ_SAMPLE_STATE;
						node->value->info->view_state		= NOT_NEW_VIEW_STATE;

						data_values->insertInRear(node->value->value);
						sample_infos->insertInRear(node->value->info);
					}
					else // ���� ��ŭ ��� �о����� while���� Ż��... ���� Ż�� ���� RTPS�� �̿��ؼ� ������ �κа� return ó���� ��
					{
						break;
					}
				}
			}
		}
		else if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == TOPIC_PRESENTATION_QOS)
		{
			if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.ordered_access == true)
			{
				if(((DataReaderQos*)qos)->destination_order.kind == BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS)
				{
					while(data_sample_list.getSize() > i++)
					{
						node = node->rear;

						if( (node->value->info->sample_state & a_condition->sample_state_mask) && (node->value->info->view_state & a_condition->view_state_mask) &&
								(node->value->info->instance_state & a_condition->instance_state_mask) && (node->value->info->instance_handle > previous_handle))
						{
							if(max_sample > data_values->getSize())
							{
								node->value->info->sample_state	= READ_SAMPLE_STATE;
								node->value->info->view_state		= NOT_NEW_VIEW_STATE;

								data_values->insertInRear(node->value->value);
								sample_infos->insertInRear(node->value->info);
							}
							else
							{
								break;
							}
						}
					}
				}
				else // Source_timestamp�� ����Ͽ� List�� ����
				{
					Time_t 	cTime; // Source_timestamp���� ���ϱ� ���� ����
					bool 		cFlag; // �о��� ���������� Ȯ��
					//int			count[max_sample]; // �о�� �� �����͵��� ��� Index�� ����
					int*		count	= new int[max_sample];
					int	 		j ;

					// ��������� �ý��� �����ϰ� 0�ʿ� �����͸� ��� ������ 0,0���� �ʱ�ȭ �Ͽ���
					cTime.sec			= 0;
					cTime.nanosec		= 0;

					// ���� ���� Source_Timestamp ���� ã��
					while(data_sample_list.getSize() > i++)
					{
						node = node->rear;

						if( (node->value->info->sample_state & a_condition->sample_state_mask) && (node->value->info->view_state & a_condition->view_state_mask) &&
								(node->value->info->instance_state & a_condition->instance_state_mask) && (node->value->info->instance_handle > previous_handle))
						{
							if((cTime.sec == 0) && (cTime.nanosec == 0)) // �ʱⰪ ����
							{
								cTime.sec			= node->value->info->source_timestamp.sec;
								cTime.nanosec		= node->value->info->source_timestamp.nanosec;
								count[0]				= i;
							}
							else if(cTime.sec > node->value->info->source_timestamp.sec)
							{
								cTime.sec			= node->value->info->source_timestamp.sec;
								cTime.nanosec		= node->value->info->source_timestamp.nanosec;
								count[0]				= i;
							}
							else if(cTime.sec == node->value->info->source_timestamp.sec)
							{
								if(cTime.nanosec > node->value->info->source_timestamp.nanosec)
								{
									cTime.sec			= node->value->info->source_timestamp.sec;
									cTime.nanosec		= node->value->info->source_timestamp.nanosec;
									count[0]				= i;
								}
							}
						}
					}

					i		= 0;
					j		= 0;
					cFlag	= true;
					cTime.sec			= 0;
					cTime.nanosec		= 0;

					while(max_sample-1 > j++)
					{
						node = data_sample_list.getNodeByIndex(0); // ��� ��带 �˻��ϱ� ���ؼ� ����� ó������ ����

						while(data_sample_list.getSize() > i++)
						{
							node = node->rear;

							if( (node->value->info->sample_state & a_condition->sample_state_mask) && (node->value->info->view_state & a_condition->view_state_mask) &&
									(node->value->info->instance_state & a_condition->instance_state_mask) && (node->value->info->instance_handle > previous_handle))
							{
								int		k;
								for(k = 0; k < j; k++) // ������ ��ϵǾ� �ִ� ���̸� ���� �ʿ䰡 �����Ƿ� üũ
								{
									if(count[k] == i) // ������ ��� �Ǿ����� Flag���� ����
									{
										cFlag	= false;
										break;
									}
								}

								if((cTime.sec == 0) && (cTime.nanosec == 0) && cFlag)
								{
									cTime.sec			= node->value->info->source_timestamp.sec;
									cTime.nanosec		= node->value->info->source_timestamp.nanosec;
									count[j]				= i;
								}

								if(cTime.sec > node->value->info->source_timestamp.sec && cFlag)
								{
									cTime.sec 			= node->value->info->source_timestamp.sec;
									cTime.nanosec		= node->value->info->source_timestamp.nanosec;
									count[j]				= i;
								}
								else if(cTime.sec == node->value->info->source_timestamp.sec && cFlag)
								{
									if(cTime.nanosec > node->value->info->source_timestamp.nanosec)
									{
										cTime.sec 			= node->value->info->source_timestamp.sec;
										cTime.nanosec		= node->value->info->source_timestamp.nanosec;
										count[j]				= i;
									}
								}
							}

							cFlag = true;
						}
					}

					for(i = 0; i < max_sample; i++) // ������ ������ ����� Index�� �̿��ؼ� �����͸� ����
					{
						node = data_sample_list.getNodeByIndex(count[i]);

						node->value->info->sample_state	= READ_SAMPLE_STATE;
						node->value->info->view_state		= NOT_NEW_VIEW_STATE;

						data_values->insertInRear(node->value->value);
						sample_infos->insertInRear(node->value->info);
					}
				}
			}
			else // Ordered_access = false
			 {
				while(data_sample_list.getSize() > i++)
				{
					node = node->rear;

					if( (node->value->info->sample_state & a_condition->sample_state_mask) && (node->value->info->view_state & a_condition->view_state_mask) &&
							(node->value->info->instance_state & a_condition->instance_state_mask) && (node->value->info->instance_handle > previous_handle))
					{
						if(max_sample > data_values->getSize())
						{
							node->value->info->sample_state	= READ_SAMPLE_STATE;
							node->value->info->view_state		= NOT_NEW_VIEW_STATE;

							data_values->insertInRear(node->value->value);
							sample_infos->insertInRear(node->value->info);
						}
						else
						{
							break;
						}
					}
				}
			}
		}
		else if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == GROUP_PRESENTATION_QOS)
		{
			// presentation qos�� group�̸� ordered_access�� true�� ��쿡�� �ִ� 1���� ������ ��ȯ�Ѵ�.
			 if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.ordered_access == true)
			 {
				 if(((DataReaderQos*)qos)->destination_order.kind == BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS)
				 {
					 while(data_sample_list.getSize() > i++)
					 {
						 node = node->rear;

							if( (node->value->info->sample_state & a_condition->sample_state_mask) && (node->value->info->view_state & a_condition->view_state_mask) &&
									(node->value->info->instance_state & a_condition->instance_state_mask) && (node->value->info->instance_handle > previous_handle))
						 {
							 node->value->info->sample_state	= READ_SAMPLE_STATE;
							 node->value->info->view_state		= NOT_NEW_VIEW_STATE;

							 data_values->insertInRear(node->value->value);
							 sample_infos->insertInRear(node->value->info);

							 break;
						 }
					 }
				 }
				 else // destination_order.kind == SOURCE_TIMESTAMP
				 {
					 Time_t 	cTime;
					 int		count; // Data, SampleInfo �ϳ��� ������ �ǹǷ� �迭�� �̿����� ����

					 // ��������� �ý��� �����ϰ� 0�ʿ� �����͸� �� �� �����״� 0,0���� �ʱ�ȭ �Ͽ���
					 cTime.sec			= 0;
					 cTime.nanosec	= 0;

					 // ���� ���� ���� ã��
					 while(data_sample_list.getSize() > i++)
					 {
						 node = node->rear;

							if( (node->value->info->sample_state & a_condition->sample_state_mask) && (node->value->info->view_state & a_condition->view_state_mask) &&
									(node->value->info->instance_state & a_condition->instance_state_mask) && (node->value->info->instance_handle > previous_handle))
						 {
							 if((cTime.sec == 0) && (cTime.nanosec == 0))
							 {
								 cTime.sec			= node->value->info->source_timestamp.sec;
								 cTime.nanosec	= node->value->info->source_timestamp.nanosec;
								 count				= i;
							 }
							 else if(cTime.sec > node->value->info->source_timestamp.sec)
							 {
								 cTime.sec			= node->value->info->source_timestamp.sec;
								 cTime.nanosec	= node->value->info->source_timestamp.nanosec;
								 count				= i;
							 }
							 else if(cTime.sec == node->value->info->source_timestamp.sec)
							 {
								 if(cTime.nanosec > node->value->info->source_timestamp.nanosec)
								 {
									 cTime.sec			= node->value->info->source_timestamp.sec;
									 cTime.nanosec	= node->value->info->source_timestamp.nanosec;
									 count				= i;
								 }
							 }
						 }
					 }

					 node = data_sample_list.getNodeByIndex(count);

					 node->value->info->sample_state	= READ_SAMPLE_STATE;
					 node->value->info->view_state		= NOT_NEW_VIEW_STATE;

					 data_values->insertInRear(node->value->value);
					 sample_infos->insertInRear(node->value->info);
				 }
			 }
			 else // Ordered_access = false
			 {
				 while(data_sample_list.getSize() > i++)
				 {
					 node = node->rear;

						if( (node->value->info->sample_state & a_condition->sample_state_mask) && (node->value->info->view_state & a_condition->view_state_mask) &&
								(node->value->info->instance_state & a_condition->instance_state_mask) && (node->value->info->instance_handle > previous_handle))
					 {
						 if(max_sample > data_values->getSize())
						 {
							 node->value->info->sample_state	= READ_SAMPLE_STATE;
							 node->value->info->view_state		= NOT_NEW_VIEW_STATE;

							 data_values->insertInRear(node->value->value);
							 sample_infos->insertInRear(node->value->info);
						 }
						 else
						 {
							 break;
						 }
					 }
				 }
			 }
		}

		read_flag	= false;

		// �����͸� ������ ����Ʈ�� ũ�Ⱑ 0���� ũ�ٸ� �����͸� ���� ����.. ������ ���ʿ� �Ѿ���� ����Ʈ�� ũ�Ⱑ 0�̶�� ���� ������ �� �����Ƿ� ����� �ʿ���
		// �����ϰ� �ϸ� ���ʿ� ����Ʈ ũ�⸦ �����Ͽ��ٰ� ���ϰų� �����͸� �д� ���� result = RETCODE_OK�� �ִ� ����� �ְ���..
		if(data_values->getSize() > 0)
		{
			result		= RETCODE_OK;
		}
		else // �����͸� ã�� ��������(�ƿ� �����͸� �ϳ��� ���� ���� ���)
		{
			result		 = RETCODE_NO_DATA;
		}

		// rtps ���� �ʿ�
		// ���⼭ sample_info�� �پ���?? rank ���� ��������� ��
	}

	ReturnCode_t		DataReader::take_instance(DataList* data_values, SampleInfoList* sample_infos, long max_sample, InstanceHandle_t a_handle, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states)
	{
		ReturnCode_t	result	= RETCODE_OK;

		bool	Invalid_Flag = false;

		if(!is_enabled)
		{
			result		=	RETCODE_NOT_ENABLED;
			return	result;
		}

		if(max_sample == 0)		// Zero-copy�� ��� return_loan�� ȣ���ؾ� �� , �� �� ���� �ʿ�
		{
			result		= return_loan(data_values, sample_infos);
			return 	result;
		}

		read_flag	 = true; // READING - Subscriber���� �����͸� �д� ������ �Ǻ��ϱ� ���ؼ� �߰�.

		int							i		= 0;
		Node<DataSample>* node = data_sample_list.getNodeByIndex(0);


		if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == INSTANCE_PRESENTATION_QOS)
		{
			while(data_sample_list.getSize() > i++)
			{
				node = node->rear;

				if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
						(node->value->info->instance_state & instance_states) && (node->value->info->instance_handle == a_handle))
				{
					Invalid_Flag = true;

					if(max_sample > data_values->getSize())
					{
						node->value->info->sample_state	= READ_SAMPLE_STATE;
						node->value->info->view_state		= NOT_NEW_VIEW_STATE;

						data_values->insertInRear(node->value->value);
						sample_infos->insertInRear(node->value->info);

						data_sample_list.cutNode(node); // ���� ��� ����
						delete(node->value);
						delete(node);
					}
					else
					{
						break;
					}
				}
			}
		}
		else if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == TOPIC_PRESENTATION_QOS)
		{
			if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.ordered_access == true)
			{
				if(((DataReaderQos*)qos)->destination_order.kind == BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS)
				{
					while(data_sample_list.getSize() > i++)
					{
						node = node->rear;

						if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
								(node->value->info->instance_state & instance_states) && (node->value->info->instance_handle == a_handle))
						{
							Invalid_Flag = true;

							if(max_sample > data_values->getSize())
							{
								node->value->info->sample_state	= READ_SAMPLE_STATE;
								node->value->info->view_state		= NOT_NEW_VIEW_STATE;

								data_values->insertInRear(node->value->value);
								sample_infos->insertInRear(node->value->info);

								data_sample_list.cutNode(node); // ���� ��� ����
								delete(node->value);
								delete(node);
							}
							else
							{
								break;
							}
						}
					}
				}
				else // Source_timestamp�� ����Ͽ� List�� ����
				{
					Time_t 	cTime;
					int			count;
					int			j;

					// ��������� �ý��� �����ϰ� 0�ʿ� �����͸� ��� ������ 0,0���� �ʱ�ȭ �Ͽ���
					cTime.sec			= 0;
					cTime.nanosec		= 0;
					j						= 0;

					while(max_sample > j++)
					{
						while(data_sample_list.getSize() > i++)
						{
							node = node->rear;

							if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
									(node->value->info->instance_state & instance_states) && (node->value->info->instance_handle == a_handle))
							{
								Invalid_Flag = true;

								if((cTime.sec == 0) && (cTime.nanosec == 0))
								{
									cTime.sec			= node->value->info->source_timestamp.sec;
									cTime.nanosec		= node->value->info->source_timestamp.nanosec;
									count					= i;
								}
								else if(cTime.sec > node->value->info->source_timestamp.sec)
								{
									cTime.sec			= node->value->info->source_timestamp.sec;
									cTime.nanosec		= node->value->info->source_timestamp.nanosec;
									count					= i;
								}
								else if(cTime.sec == node->value->info->source_timestamp.sec)
								{
									if(cTime.nanosec > node->value->info->source_timestamp.nanosec)
									{
										cTime.sec			= node->value->info->source_timestamp.sec;
										cTime.nanosec		= node->value->info->source_timestamp.nanosec;
										count					= i;
									}
								}
							}
						}

						node = data_sample_list.getNodeByIndex(count);

						node->value->info->sample_state	= READ_SAMPLE_STATE;
						node->value->info->view_state		= NOT_NEW_VIEW_STATE;

						data_values->insertInRear(node->value->value);
						sample_infos->insertInRear(node->value->info);

						data_sample_list.cutNode(node);
						delete(node->value);
						delete(node);

						node = data_sample_list.getNodeByIndex(0);

						i						= 0;
						cTime.sec			= 0;
						cTime.nanosec		= 0;
					}
				}
			}
			else // false
			 {
				while(data_sample_list.getSize() > i++)
				{
					node = node->rear;

					if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
							(node->value->info->instance_state & instance_states) && (node->value->info->instance_handle == a_handle))
					{
						Invalid_Flag = true;

						if(max_sample > data_values->getSize())
						{
							node->value->info->sample_state	= READ_SAMPLE_STATE;
							node->value->info->view_state		= NOT_NEW_VIEW_STATE;

							data_values->insertInRear(node->value->value);
							sample_infos->insertInRear(node->value->info);

							data_sample_list.cutNode(node);
							delete(node->value);
							delete(node);
						}
						else
						{
							break;
						}
					}
				}
			}
		}
		else if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == GROUP_PRESENTATION_QOS)
		{
			// presentation qos�� group�̸� ordered_access�� true�� ��쿡�� �ִ� 1���� ������ ��ȯ�Ѵ�.
			 if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.ordered_access == true)
			 {
				 if(((DataReaderQos*)qos)->destination_order.kind == BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS)
				 {
					 while(data_sample_list.getSize() > i++)
					 {
						 node = node->rear;

						 if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
								 (node->value->info->instance_state & instance_states) && (node->value->info->instance_handle == a_handle))
						 {
							 Invalid_Flag = true;

							 node->value->info->sample_state	= READ_SAMPLE_STATE;
							 node->value->info->view_state		= NOT_NEW_VIEW_STATE;

							 data_values->insertInRear(node->value->value);
							 sample_infos->insertInRear(node->value->info);

							 data_sample_list.cutNode(node);
							 delete(node->value);
							 delete(node);

							 break;
						 }
					 }
				 }
				 else // destination_order.kind == SOURCE_TIMESTAMP
				 {
					 Time_t 	cTime;
					 int			count;

					 // ��������� �ý��� �����ϰ� 0�ʿ� �����͸� �� �� �����״� 0,0���� �ʱ�ȭ �Ͽ���
					 cTime.sec			= 0;
					 cTime.nanosec	= 0;

					 // ���� ���� ���� ã��
					 while(data_sample_list.getSize() > i++)
					 {
						 node = node->rear;

						 if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
								 (node->value->info->instance_state & instance_states) && (node->value->info->instance_handle == a_handle))
						 {
							 Invalid_Flag = true;

							 if((cTime.sec == 0) && (cTime.nanosec == 0))
							 {
								 cTime.sec			= node->value->info->source_timestamp.sec;
								 cTime.nanosec	= node->value->info->source_timestamp.nanosec;
								 count				= i;
							 }
							 else if(cTime.sec > node->value->info->source_timestamp.sec)
							 {
								 cTime.sec			= node->value->info->source_timestamp.sec;
								 cTime.nanosec	= node->value->info->source_timestamp.nanosec;
								 count				= i;
							 }
							 else if(cTime.sec == node->value->info->source_timestamp.sec)
							 {
								 if(cTime.nanosec > node->value->info->source_timestamp.nanosec)
								 {
									 cTime.sec			= node->value->info->source_timestamp.sec;
									 cTime.nanosec	= node->value->info->source_timestamp.nanosec;
									 count				= i;
								 }
							 }
						 }
					 }

					 node = data_sample_list.getNodeByIndex(count);

					 node->value->info->sample_state	= READ_SAMPLE_STATE;
					 node->value->info->view_state		= NOT_NEW_VIEW_STATE;

					 data_values->insertInRear(node->value->value);
					 sample_infos->insertInRear(node->value->info);

					 data_sample_list.cutNode(node);
					 delete(node->value);
					 delete(node);
				 }
			 }
			 else // Ordered_access = false
			 {
				 while(data_sample_list.getSize() > i++)
				 {
					 node = node->rear;

					 if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
							 (node->value->info->instance_state & instance_states) && (node->value->info->instance_handle == a_handle))
					 {
						 Invalid_Flag = true;

						 if(max_sample > data_values->getSize())
						 {
							 node->value->info->sample_state	= READ_SAMPLE_STATE;
							 node->value->info->view_state		= NOT_NEW_VIEW_STATE;

							 data_values->insertInRear(node->value->value);
							 sample_infos->insertInRear(node->value->info);

							 data_sample_list.cutNode(node);
							 delete(node->value);
							 delete(node);
						 }
						 else
						 {
							 break;
						 }
					 }
				 }
			 }
		}

		read_flag	= false;

		if(Invalid_Flag == false)
		{
			result		= RETCODE_BAD_PARAMETER;
		}

		if(data_values->getSize() > 0)
		{
			result		= RETCODE_OK;
		}
		else
		{
			result		 = RETCODE_NO_DATA;
		}

		// rtps ���� �ʿ�
		// ���⼭ sample_info�� �پ���?? rank ���� ��������� ��

	}

	ReturnCode_t		DataReader::take_next_instance(DataList* data_values, SampleInfoList* sample_infos, long max_sample, InstanceHandle_t previous_handle, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states)
	{
		ReturnCode_t	result	= RETCODE_OK;

		bool	Invalid_Flag = false;

		if(!is_enabled)
		{
			result		=	RETCODE_NOT_ENABLED;
			return	result;
		}

		if(max_sample == 0)		// Zero-copy�� ��� return_loan�� ȣ���ؾ� �� , �� �� ���� �ʿ�
		{
			result		= return_loan(data_values, sample_infos);
			return 	result;
		}

		read_flag = true;

		int							i		= 0;
		Node<DataSample>* node	= data_sample_list.getNodeByIndex(0);

		InstanceHandle_t	smallest_handle = 0;


		// previous_handle���ٴ� ũ�� ���� ���� Handle�� ã��
		while(data_sample_list.getSize() > i++)
		{
			node	= node->rear;

			if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
					(node->value->info->instance_state & instance_states) && (node->value->info->instance_handle > previous_handle))
			{
				if(smallest_handle == 0)
				{
					smallest_handle = node->value->info->instance_handle;
				}
				else if(smallest_handle > node->value->info->instance_handle)
				{
					smallest_handle = node->value->info->instance_handle;
				}
			}
		}

		node	= data_sample_list.getNodeByIndex(0);
		i		= 0;


		if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == INSTANCE_PRESENTATION_QOS)
		{
			while(data_sample_list.getSize() > i++)
			{
				node = node->rear;

				if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
						(node->value->info->instance_state & instance_states) && (node->value->info->instance_handle == smallest_handle))
				{
					Invalid_Flag = true;

					if(max_sample > data_values->getSize())
					{
						node->value->info->sample_state	= READ_SAMPLE_STATE;
						node->value->info->view_state		= NOT_NEW_VIEW_STATE;

						data_values->insertInRear(node->value->value);
						sample_infos->insertInRear(node->value->info);

						data_sample_list.cutNode(node); // ���� ��� ����
						delete(node->value);
						delete(node);
					}
					else
					{
						break;
					}
				}
			}
		}
		else if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == TOPIC_PRESENTATION_QOS)
		{
			if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.ordered_access == true)
			{
				if(((DataReaderQos*)qos)->destination_order.kind == BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS)
				{
					while(data_sample_list.getSize() > i++)
					{
						node = node->rear;

						if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
								(node->value->info->instance_state & instance_states) && (node->value->info->instance_handle == smallest_handle))
						{
							Invalid_Flag = true;

							if(max_sample > data_values->getSize())
							{
								node->value->info->sample_state	= READ_SAMPLE_STATE;
								node->value->info->view_state		= NOT_NEW_VIEW_STATE;

								data_values->insertInRear(node->value->value);
								sample_infos->insertInRear(node->value->info);

								data_sample_list.cutNode(node); // ���� ��� ����
								delete(node->value);
								delete(node);
							}
							else
							{
								break;
							}
						}
					}
				}
				else // Source_timestamp�� ����Ͽ� List�� ����
				{
					Time_t 	cTime;
					int			count;
					int			j;

					// ��������� �ý��� �����ϰ� 0�ʿ� �����͸� ��� ������ 0,0���� �ʱ�ȭ �Ͽ���
					cTime.sec			= 0;
					cTime.nanosec		= 0;
					j						= 0;

					while(max_sample > j++)
					{
						while(data_sample_list.getSize() > i++)
						{
							node = node->rear;

							if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
									(node->value->info->instance_state & instance_states) && (node->value->info->instance_handle == smallest_handle))
							{
								Invalid_Flag = true;

								if((cTime.sec == 0) && (cTime.nanosec == 0))
								{
									cTime.sec			= node->value->info->source_timestamp.sec;
									cTime.nanosec		= node->value->info->source_timestamp.nanosec;
									count					= i;
								}
								else if(cTime.sec > node->value->info->source_timestamp.sec)
								{
									cTime.sec			= node->value->info->source_timestamp.sec;
									cTime.nanosec		= node->value->info->source_timestamp.nanosec;
									count					= i;
								}
								else if(cTime.sec == node->value->info->source_timestamp.sec)
								{
									if(cTime.nanosec > node->value->info->source_timestamp.nanosec)
									{
										cTime.sec			= node->value->info->source_timestamp.sec;
										cTime.nanosec		= node->value->info->source_timestamp.nanosec;
										count					= i;
									}
								}
							}
						}

						node = data_sample_list.getNodeByIndex(count);

						node->value->info->sample_state	= READ_SAMPLE_STATE;
						node->value->info->view_state		= NOT_NEW_VIEW_STATE;

						data_values->insertInRear(node->value->value);
						sample_infos->insertInRear(node->value->info);

						data_sample_list.cutNode(node);
						delete(node->value);
						delete(node);

						node = data_sample_list.getNodeByIndex(0);

						i						= 0;
						cTime.sec			= 0;
						cTime.nanosec		= 0;
					}
				}
			}
			else // false
			 {
				while(data_sample_list.getSize() > i++)
				{
					node = node->rear;

					if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
							(node->value->info->instance_state & instance_states) && (node->value->info->instance_handle == smallest_handle))
					{
						Invalid_Flag = true;

						if(max_sample > data_values->getSize())
						{
							node->value->info->sample_state	= READ_SAMPLE_STATE;
							node->value->info->view_state		= NOT_NEW_VIEW_STATE;

							data_values->insertInRear(node->value->value);
							sample_infos->insertInRear(node->value->info);

							data_sample_list.cutNode(node);
							delete(node->value);
							delete(node);
						}
						else
						{
							break;
						}
					}
				}
			 }
		}
		else if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == GROUP_PRESENTATION_QOS)
		{
			// presentation qos�� group�̸� ordered_access�� true�� ��쿡�� �ִ� 1���� ������ ��ȯ�Ѵ�.
			if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.ordered_access == true)
			{
				if(((DataReaderQos*)qos)->destination_order.kind == BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS)
				{
					while(data_sample_list.getSize() > i++)
					{
						node = node->rear;

						if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
								(node->value->info->instance_state & instance_states) && (node->value->info->instance_handle == smallest_handle))
						{
							Invalid_Flag = true;

							node->value->info->sample_state	= READ_SAMPLE_STATE;
							node->value->info->view_state		= NOT_NEW_VIEW_STATE;

							data_values->insertInRear(node->value->value);
							sample_infos->insertInRear(node->value->info);

							data_sample_list.cutNode(node);
							delete(node->value);
							delete(node);

							break;
						 }
					 }
				 }
				 else // destination_order.kind == SOURCE_TIMESTAMP
				 {
					 Time_t 	cTime;
					 int			count;

					 // ��������� �ý��� �����ϰ� 0�ʿ� �����͸� �� �� �����״� 0,0���� �ʱ�ȭ �Ͽ���
					 cTime.sec			= 0;
					 cTime.nanosec	= 0;

					 // ���� ���� ���� ã��
					 while(data_sample_list.getSize() > i++)
					 {
						 node = node->rear;

						 if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
								 (node->value->info->instance_state & instance_states) && (node->value->info->instance_handle == smallest_handle))
						 {
							 Invalid_Flag = true;

							 if((cTime.sec == 0) && (cTime.nanosec == 0))
							 {
								 cTime.sec			= node->value->info->source_timestamp.sec;
								 cTime.nanosec	= node->value->info->source_timestamp.nanosec;
								 count				= i;
							 }
							 else if(cTime.sec > node->value->info->source_timestamp.sec)
							 {
								 cTime.sec			= node->value->info->source_timestamp.sec;
								 cTime.nanosec	= node->value->info->source_timestamp.nanosec;
								 count				= i;
							 }
							 else if(cTime.sec == node->value->info->source_timestamp.sec)
							 {
								 if(cTime.nanosec > node->value->info->source_timestamp.nanosec)
								 {
									 cTime.sec			= node->value->info->source_timestamp.sec;
									 cTime.nanosec	= node->value->info->source_timestamp.nanosec;
									 count				= i;
								 }
							 }
						 }
					 }

					 node = data_sample_list.getNodeByIndex(count);

					 node->value->info->sample_state	= READ_SAMPLE_STATE;
					 node->value->info->view_state		= NOT_NEW_VIEW_STATE;

					 data_values->insertInRear(node->value->value);
					 sample_infos->insertInRear(node->value->info);

					 data_sample_list.cutNode(node);
					 delete(node->value);
					 delete(node);
				 }
			 }
			 else // Ordered_access = false
			 {
				 while(data_sample_list.getSize() > i++)
				 {
					 node = node->rear;

					 if( (node->value->info->sample_state & sample_states) && (node->value->info->view_state & view_states) &&
							 (node->value->info->instance_state & instance_states) && (node->value->info->instance_handle == smallest_handle))
					 {
						 Invalid_Flag = true;

						 if(max_sample > data_values->getSize())
						 {
							 node->value->info->sample_state	= READ_SAMPLE_STATE;
							 node->value->info->view_state		= NOT_NEW_VIEW_STATE;

							 data_values->insertInRear(node->value->value);
							 sample_infos->insertInRear(node->value->info);

							 data_sample_list.cutNode(node);
							 delete(node->value);
							 delete(node);
						 }
						 else
						 {
							 break;
						 }
					 }
				 }
			 }
		}

		read_flag	= false;

		if(Invalid_Flag == false)
		{
			result		= RETCODE_BAD_PARAMETER;
		}

		if(data_values->getSize() > 0)
		{
			result		= RETCODE_OK;
		}
		else
		{
			result		 = RETCODE_NO_DATA;
		}

		// rtps ���� �ʿ�
		// ���⼭ sample_info�� �پ���?? rank ���� ��������� ��

		return	result;
	}

	ReturnCode_t		DataReader::take_next_instance_w_condition(DataList* data_values, SampleInfoList* sample_infos, long max_sample, InstanceHandle_t previous_handle, ReadCondition* a_condition)
	{
		ReturnCode_t	result	= RETCODE_OK;

		bool	Invalid_Flag = false;

		if(!is_enabled)
		{
			result		=	RETCODE_NOT_ENABLED;
			return	result;
		}

		if(max_sample == 0)		// Zero-copy�� ��� return_loan�� ȣ���ؾ� �� , �� �� ���� �ʿ�
		{
			result		= return_loan(data_values, sample_infos);
			return 	result;
		}

		read_flag = true;

		int							i		= 0;
		Node<DataSample>* node	= data_sample_list.getNodeByIndex(0);

		InstanceHandle_t	smallest_handle = 0;


		// previous_handle���ٴ� ũ�� ���� ���� Handle�� ã��
		while(data_sample_list.getSize() > i++)
		{
			node	= node->rear;

			if( (node->value->info->sample_state & a_condition->sample_state_mask) && (node->value->info->view_state & a_condition->view_state_mask) &&
					(node->value->info->instance_state & a_condition->instance_state_mask) && (node->value->info->instance_handle > previous_handle))
			{
				if(smallest_handle == 0)
				{
					smallest_handle = node->value->info->instance_handle;
				}
				else if(smallest_handle > node->value->info->instance_handle)
				{
					smallest_handle = node->value->info->instance_handle;
				}
			}
		}

		node	= data_sample_list.getNodeByIndex(0);
		i		= 0;


		if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == INSTANCE_PRESENTATION_QOS)
		{
			while(data_sample_list.getSize() > i++)
			{
				node = node->rear;

				if( (node->value->info->sample_state & a_condition->sample_state_mask) && (node->value->info->view_state & a_condition->view_state_mask) &&
						(node->value->info->instance_state & a_condition->instance_state_mask) && (node->value->info->instance_handle > previous_handle))
				{
					Invalid_Flag = true;

					if(max_sample > data_values->getSize())
					{
						node->value->info->sample_state	= READ_SAMPLE_STATE;
						node->value->info->view_state		= NOT_NEW_VIEW_STATE;

						data_values->insertInRear(node->value->value);
						sample_infos->insertInRear(node->value->info);

						data_sample_list.cutNode(node); // ���� ��� ����
						delete(node->value);
						delete(node);
					}
					else
					{
						break;
					}
				}
			}
		}
		else if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == TOPIC_PRESENTATION_QOS)
		{
			if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.ordered_access == true)
			{
				if(((DataReaderQos*)qos)->destination_order.kind == BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS)
				{
					while(data_sample_list.getSize() > i++)
					{
						node = node->rear;

						if( (node->value->info->sample_state & a_condition->sample_state_mask) && (node->value->info->view_state & a_condition->view_state_mask) &&
								(node->value->info->instance_state & a_condition->instance_state_mask) && (node->value->info->instance_handle > previous_handle))
						{
							Invalid_Flag = true;

							if(max_sample > data_values->getSize())
							{
								node->value->info->sample_state	= READ_SAMPLE_STATE;
								node->value->info->view_state		= NOT_NEW_VIEW_STATE;

								data_values->insertInRear(node->value->value);
								sample_infos->insertInRear(node->value->info);

								data_sample_list.cutNode(node); // ���� ��� ����
								delete(node->value);
								delete(node);
							}
							else
							{
								break;
							}
						}
					}
				}
				else // Source_timestamp�� ����Ͽ� List�� ����
				{
					Time_t 	cTime;
					int			count;
					int			j;

					// ��������� �ý��� �����ϰ� 0�ʿ� �����͸� ��� ������ 0,0���� �ʱ�ȭ �Ͽ���
					cTime.sec			= 0;
					cTime.nanosec		= 0;
					j						= 0;

					while(max_sample > j++)
					{
						while(data_sample_list.getSize() > i++)
						{
							node = node->rear;

							if( (node->value->info->sample_state & a_condition->sample_state_mask) && (node->value->info->view_state & a_condition->view_state_mask) &&
									(node->value->info->instance_state & a_condition->instance_state_mask) && (node->value->info->instance_handle > previous_handle))
							{
								Invalid_Flag = true;

								if((cTime.sec == 0) && (cTime.nanosec == 0))
								{
									cTime.sec			= node->value->info->source_timestamp.sec;
									cTime.nanosec		= node->value->info->source_timestamp.nanosec;
									count					= i;
								}
								else if(cTime.sec > node->value->info->source_timestamp.sec)
								{
									cTime.sec			= node->value->info->source_timestamp.sec;
									cTime.nanosec		= node->value->info->source_timestamp.nanosec;
									count					= i;
								}
								else if(cTime.sec == node->value->info->source_timestamp.sec)
								{
									if(cTime.nanosec > node->value->info->source_timestamp.nanosec)
									{
										cTime.sec			= node->value->info->source_timestamp.sec;
										cTime.nanosec		= node->value->info->source_timestamp.nanosec;
										count					= i;
									}
								}
							}
						}

						node = data_sample_list.getNodeByIndex(count);

						node->value->info->sample_state	= READ_SAMPLE_STATE;
						node->value->info->view_state		= NOT_NEW_VIEW_STATE;

						data_values->insertInRear(node->value->value);
						sample_infos->insertInRear(node->value->info);

						data_sample_list.cutNode(node);
						delete(node->value);
						delete(node);

						node = data_sample_list.getNodeByIndex(0);

						i						= 0;
						cTime.sec			= 0;
						cTime.nanosec		= 0;
					}
				}
			}
			else // false
			 {
				while(data_sample_list.getSize() > i++)
				{
					node = node->rear;

					if( (node->value->info->sample_state & a_condition->sample_state_mask) && (node->value->info->view_state & a_condition->view_state_mask) &&
							(node->value->info->instance_state & a_condition->instance_state_mask) && (node->value->info->instance_handle > previous_handle))
					{
						Invalid_Flag = true;

						if(max_sample > data_values->getSize())
						{
							node->value->info->sample_state	= READ_SAMPLE_STATE;
							node->value->info->view_state		= NOT_NEW_VIEW_STATE;

							data_values->insertInRear(node->value->value);
							sample_infos->insertInRear(node->value->info);

							data_sample_list.cutNode(node);
							delete(node->value);
							delete(node);
						}
						else
						{
							break;
						}
					}
				}
			 }
		}
		else if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.access_scope == GROUP_PRESENTATION_QOS)
		{
			// presentation qos�� group�̸� ordered_access�� true�� ��쿡�� �ִ� 1���� ������ ��ȯ�Ѵ�.
			if(((SubscriberQos*)this->get_subscriber()->qos)->presentation.ordered_access == true)
			{
				if(((DataReaderQos*)qos)->destination_order.kind == BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS)
				{
					while(data_sample_list.getSize() > i++)
					{
						node = node->rear;

						if( (node->value->info->sample_state & a_condition->sample_state_mask) && (node->value->info->view_state & a_condition->view_state_mask) &&
								(node->value->info->instance_state & a_condition->instance_state_mask) && (node->value->info->instance_handle > previous_handle))
						{
							Invalid_Flag = true;

							node->value->info->sample_state	= READ_SAMPLE_STATE;
							node->value->info->view_state		= NOT_NEW_VIEW_STATE;

							data_values->insertInRear(node->value->value);
							sample_infos->insertInRear(node->value->info);

							data_sample_list.cutNode(node);
							delete(node->value);
							delete(node);

							break;
						 }
					 }
				 }
				 else // destination_order.kind == SOURCE_TIMESTAMP
				 {
					 Time_t 	cTime;
					 int			count;

					 // ��������� �ý��� �����ϰ� 0�ʿ� �����͸� �� �� �����״� 0,0���� �ʱ�ȭ �Ͽ���
					 cTime.sec			= 0;
					 cTime.nanosec	= 0;

					 // ���� ���� ���� ã��
					 while(data_sample_list.getSize() > i++)
					 {
						 node = node->rear;

						 if( (node->value->info->sample_state & a_condition->sample_state_mask) && (node->value->info->view_state & a_condition->view_state_mask) &&
								 (node->value->info->instance_state & a_condition->instance_state_mask) && (node->value->info->instance_handle > previous_handle))
						 {
							 Invalid_Flag = true;

							 if((cTime.sec == 0) && (cTime.nanosec == 0))
							 {
								 cTime.sec			= node->value->info->source_timestamp.sec;
								 cTime.nanosec	= node->value->info->source_timestamp.nanosec;
								 count				= i;
							 }
							 else if(cTime.sec > node->value->info->source_timestamp.sec)
							 {
								 cTime.sec			= node->value->info->source_timestamp.sec;
								 cTime.nanosec	= node->value->info->source_timestamp.nanosec;
								 count				= i;
							 }
							 else if(cTime.sec == node->value->info->source_timestamp.sec)
							 {
								 if(cTime.nanosec > node->value->info->source_timestamp.nanosec)
								 {
									 cTime.sec			= node->value->info->source_timestamp.sec;
									 cTime.nanosec	= node->value->info->source_timestamp.nanosec;
									 count				= i;
								 }
							 }
						 }
					 }

					 node = data_sample_list.getNodeByIndex(count);

					 node->value->info->sample_state	= READ_SAMPLE_STATE;
					 node->value->info->view_state		= NOT_NEW_VIEW_STATE;

					 data_values->insertInRear(node->value->value);
					 sample_infos->insertInRear(node->value->info);

					 data_sample_list.cutNode(node);
					 delete(node->value);
					 delete(node);
				 }
			 }
			 else // Ordered_access = false
			 {
				 while(data_sample_list.getSize() > i++)
				 {
					 node = node->rear;

					 if( (node->value->info->sample_state & a_condition->sample_state_mask) && (node->value->info->view_state & a_condition->view_state_mask) &&
							 (node->value->info->instance_state & a_condition->instance_state_mask) && (node->value->info->instance_handle > previous_handle))
					 {
						 Invalid_Flag = true;

						 if(max_sample > data_values->getSize())
						 {
							 node->value->info->sample_state	= READ_SAMPLE_STATE;
							 node->value->info->view_state		= NOT_NEW_VIEW_STATE;

							 data_values->insertInRear(node->value->value);
							 sample_infos->insertInRear(node->value->info);

							 data_sample_list.cutNode(node);
							 delete(node->value);
							 delete(node);
						 }
						 else
						 {
							 break;
						 }
					 }
				 }
			 }
		}

		read_flag	= false;

		if(Invalid_Flag == false)
		{
			result		= RETCODE_BAD_PARAMETER;
		}

		if(data_values->getSize() > 0)
		{
			result		= RETCODE_OK;
		}
		else
		{
			result		= RETCODE_NO_DATA;
		}

		// rtps ���� �ʿ�
		// ���⼭ sample_info�� �پ���?? rank ���� ��������� ��

		return	result;
	}

	ReturnCode_t		DataReader::return_loan(DataList* data_values, SampleInfoList* sample_infos)
	{
		ReturnCode_t	result	= RETCODE_OK;

		// read or take�� ���� ��ȯ�Ǵ� pair�� ��ġ�ؾ� �Ѵ�. ( pair -> data_values, sample_infos)
		// ���� DataReader�� ���� ��ȯ �Ǿ�� �Ѵ�.
		// �� 2���� ������ �ƴ� ��� PRECONDITION_NOT_MET

		// return_loan �Լ��� ����
		// CNU_DDS�� ��� �迭�� �̿����� �����Ƿ� ũ�⿡ ���� ������ ����.. Zero-copy�� ���ٴ� �Ҹ���?

		// Buffer loan from the DataReader to Application
		// provide Zero-Copy access to the data
		// Loan �߿��� Data �� Sample ������ �������� ����

		return	result;
	}

	// InstanceHandle_t�� ���õ� Instance Key�� �˻�
	// ���� �̿Ϸ� - key_holder�� Key�� �־���� �ϴ°� ������ key�� ����
	ReturnCode_t		DataReader::get_key_value(DDS_Data* key_holder, InstanceHandle_t handle)
	{
		ReturnCode_t	result	= RETCODE_OK;

		if(!is_enabled)
		{
			result	= RETCODE_NOT_ENABLED;
		}

		int							i		= 0;
		Node<DataSample>*	node	= data_sample_list.getNodeByIndex(0);

		while(data_sample_list.getSize() > i++)
		{
			node	= node->rear;
			if( node->value->info->instance_handle == handle)
			{

				// key_holder�� ���� �־���� ��
				result			= RETCODE_OK;
				return		result;
			}
		}

		result		= RETCODE_ERROR; // SPEC�� Error �ڵ忡 ���� ����� ���� ���� ����
		return	result;
	}


	// �Ѿ���� DDS_Data�� ��ġ�ϴ� DataSample�� InstanceHandle_t�� ��ȯ
	InstanceHandle_t	DataReader::lookup_instance(DDS_Data* instance)
	{
		InstanceHandle_t	result	= 0;

		if(!is_enabled)
		{
			result	= RETCODE_NOT_ENABLED;
		}

		int							i 		= 0;
		Node<DataSample>*	node	= data_sample_list.getNodeByIndex(0);

		while(data_sample_list.getSize() > i++)
		{
			node	= node->rear;
			if(node->value->value == instance) // ������ ��带 ã�� ��� ��� ���� �� while�� break
			{
				result		= node->value->info->instance_handle;
				return 	result;
			}
		}

		result		= NULL;
		return	result;
	}

	ReadCondition*		DataReader::create_readcondition(SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states)
	{
		ReadCondition*	result	= NULL;

		if(!is_enabled)
		{
			result		= NULL;
			return	result;
		}

		result		= new ReadCondition();

		result->sample_state_mask	= sample_states;
		result->view_state_mask		= view_states;
		result->instance_state_mask	= instance_states;
		result->related_waitset_list	= NULL;

		return	result;
	}

	// ���� �̿Ϸ�
	QueryCondition*		DataReader::create_querycondition(SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states, std::string query_expression, StringSeq* query_parameters)
	{
		QueryCondition*	result	= NULL;

		if(!is_enabled)
		{
			result		= NULL;
			return	result;
		}

		result		= new QueryCondition();

		result->sample_state_mask 	= sample_states;
		result->view_state_mask 		= view_states;
		result->instance_state_mask = instance_states;
		result->related_waitset_list	= NULL;

		// Parameter�� Expression�� 2�� �Ѿ���µ� QueryCondition Attribute�� Expression ������ ����
		// result->set_query_parameters(query_parameters);
		// What Expression ?
		return	result;
	}

	ReturnCode_t		DataReader::delete_readcondition(ReadCondition* a_condition)
	{
		ReturnCode_t	result	= RETCODE_OK;

		if(!is_enabled)
		{
			result	= RETCODE_NOT_ENABLED;
		}

		// ������ ReadCondition�� �ش� DataReader�� ���ϴ� ������ Ȯ��
		// ReadCondition�� get_datareader�� �� ���� ���¶� ������ ���Ƿ� �켱 �ּ� ó��
		/*
		if(a_condition->get_datareader() != this)
		{
			result		= RETCODE_PRECONDITION_NOT_MET;
			return	result;
		}
		*/

		int								 i	= 0;
		Node<ReadCondition>*	node	= read_condition_list.getNodeByIndex(0);

		while(read_condition_list.getSize() > i++)
		{
			node	= node->rear;
			if(node->value == a_condition) // ������ ��带 ã�� ��� ��� ���� �� while�� break
			{
				read_condition_list.cutNode(node);
				delete(node->value);
				delete(node);
				result		= RETCODE_OK;
				break;
			}
		}

		return	result;
	}

	ReturnCode_t		DataReader::get_liveliness_changed_status(LivelinessChangedStatus status)
	{
		ReturnCode_t	result	= RETCODE_OK;


		return	result;
	}

	ReturnCode_t		DataReader::get_requested_deadline_missed_status(RequestedDeadlineMissedStatus status)
	{
		ReturnCode_t	result	= RETCODE_OK;


		return	result;
	}

	ReturnCode_t		DataReader::get_requested_incompatible_qos_status(RequestedIncompatibleQosStatus status)
	{
		ReturnCode_t	result	= RETCODE_OK;

		return	result;
	}

	ReturnCode_t		DataReader::get_sample_lost_status(SampleLostStatus status)
	{
		ReturnCode_t	result	= RETCODE_OK;

		return	result;
	}

	ReturnCode_t		DataReader::get_sample_rejected_status(SampleRejectedStatus status)
	{
		ReturnCode_t	result	= RETCODE_OK;


		return	result;
	}

	ReturnCode_t		DataReader::get_subscription_matched_status(SubscriptionMatchedStatus status)
	{
		ReturnCode_t	result	= RETCODE_OK;

		return	result;
	}

	TopicDescription*	DataReader::get_topic_description(void)
	{
		TopicDescription*	result	= NULL;

		result		=	this->related_topic_description;
		return	result;
	}

	Subscriber*			DataReader::get_subscriber(void)
	{
		Subscriber*	result	= NULL;

		result		= this->related_subscriber;
		return	result;
	}

	ReturnCode_t		DataReader::delete_contained_entities(void)
	{
		ReturnCode_t	result	= RETCODE_OK;

		if(!is_enabled)
		{
			result	= RETCODE_NOT_ENABLED;
		}

		int										i	= 0;
		Node<ReadCondition>	*node	= read_condition_list.getNodeByIndex(0);

		while(read_condition_list.getSize() > i++)
		{
			node = node->rear;

			if(delete_readcondition(node->value) != RETCODE_OK)
			{
				result 	= RETCODE_PRECONDITION_NOT_MET;
				return	result;
			}
		}

		result		= RETCODE_OK;
		return	result;
	}

	ReturnCode_t		DataReader::wait_for_historical_data(Duration_t max_wait)
	{
		ReturnCode_t	result	= RETCODE_OK;

		if(!is_enabled)
		{
			result		= RETCODE_NOT_ENABLED;
			return	result;
		}

		// non-VOLATILE PERSISTENCE Qos Kind

		while(true)
		{
			if(((DataReaderQos*)qos)->durability.kind == VOLATILE_DURABILITY_QOS)
			{
				result		= RETCODE_PRECONDITION_NOT_MET; // ���� ���� -- VOLATILE�� ��� ���� ó�� ��� ����
				return	result;
			}

			// 2���� ��ƾ�� ���� - ȣ��Ǹ� �켱 block��.
			// 'historical' �����͸� ��� ������ ���� ���
			// Historical ������?�� �� �ǹ��ϴ°���..


			// �Ǵ� �ð��� ������ Ÿ�� �ƿ����� �Լ� ����
			if(max_wait.sec <= 0 && max_wait.nanosec <=0)
			{
				result		= RETCODE_TIMEOUT;
			}

		}

		return	result;
	}

	// ���� �̿Ϸ�
	// handle�� ��ġ�ϴ� ���� ã�� ���� publication_data�� � ���� �־�� �ϴ°���..
	ReturnCode_t		DataReader::get_matched_publication_data(PublicationBuiltinToipcData* publication_data, InstanceHandle_t publication_handle)
	{
		ReturnCode_t	result	= RETCODE_OK;

		if(!is_enabled)
		{
			result		= RETCODE_NOT_ENABLED;
			return	result;
		}

		int							i		= 0;
		Node<DataSample>* node = data_sample_list.getNodeByIndex(0);

		while(data_sample_list.getSize() > i++)
		{
			node = node->rear;

			if(node->value->info->publication_handle == publication_handle)
			{
				// � ������ �߰��ؾ� �ϴ°��� �𸣰���..
				result		= RETCODE_OK;
				return	 result;
			}
		}

		result		= RETCODE_BAD_PARAMETER;	// Handle�� ã�� ���ϸ� (��Ī�Ǵ� DataReader�� ���� ���) BAD_PARAMETER
		return	result;
	}

	ReturnCode_t		DataReader::get_matched_publications(InstanceHandleSeq* publication_handles)
	{
		ReturnCode_t	result	= RETCODE_OK;

		if(!is_enabled)
		{
			result		= RETCODE_NOT_ENABLED;
			return	result;
		}

		int							i		= 0;
		Node<DataSample>* node = data_sample_list.getNodeByIndex(0);

		while(data_sample_list.getSize() > i++)
		{
			node = node->rear;
			publication_handles->insertInRear(&(node->value->info->publication_handle));
		}

		result		= RETCODE_OK;
		return	result;
	}
}
