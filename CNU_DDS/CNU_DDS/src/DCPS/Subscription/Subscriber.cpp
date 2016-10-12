#include "../../../include/DCPS/Subscription/Subscriber.h"
#include "../../../include/DCPS/Subscription/DataReader.h"
#include "../../../include/DCPS/Domain/DomainParticipant.h"
#include "../../../include/DCPS/Topic/TopicDescription.h"  // Add Header
#include "../../../include/DCPS/Subscription/DataReaderListener.h" // // Add Header
#include "../../../include/Status/EntityStatus/SubscriberStatus.h"
#include "../../../include/Status/Condition/StatusCondition.h"

#include "../../../include/RTPS/Structure/Participant.h"
#include "../../../include/RTPS/Behavior/StatefulReader.h"
#include <stdio.h>

namespace CNU_DDS
{
	Subscriber::Subscriber()
	{
		status				= new SubscriberStatus();
		status_condition	= new StatusCondition();
		datareader_list		= new DataReaderSeq();

		status_condition->enabled_statuses	= 0x00;
		status_condition->related_entity	= this;

		is_enabled			= false;
		listener			= NULL;
		qos					= NULL;

		_set_default_datareader_qos();

		entity_key			= 0;
	}

	Subscriber::~Subscriber()
	{
		delete(status_condition);
		delete(status);
		delete(datareader_list);
	}

	ReturnCode_t		Subscriber::get_qos(SubscriberQos* qos_list)
	{
		ReturnCode_t	result;

		if(qos_list == NULL)
		{
			result	= RETCODE_BAD_PARAMETER;
		}
		else if(qos)
		{
			memcpy(qos_list, qos, sizeof(SubscriberQos));
			result		= RETCODE_OK;
		}
		else
		{
			result		= RETCODE_PRECONDITION_NOT_MET;	// undefined
		}

		return	result;
	}

	// ���� �̿Ϸ�
	// set_qos���� ������ Error return type�� 2���� -- INCONSISTENT or IMMUTABLE
	// ó�� ���� �Ǵ� enabled ������ inconsistent or immutable �����ϴ� ��쿡�� ������ �ƴ�
	ReturnCode_t		Subscriber::set_qos(SubscriberQos* qos_list)
	{
		ReturnCode_t	result; // Not defined

		// enabled �ϱ� ������ �����ϴ� ���� ó�� �����ؼ� �����ϴ� ��� immutable, inconsistent ���� ���� ����
		// default_datareader_qos??

		if(!is_enabled && qos == NULL)
		{
			qos	= new SubscriberQos();
			((SubscriberQos*)qos)->entity_factory.auto_enable_created_entities	= qos_list->entity_factory.auto_enable_created_entities;
			memcpy(((SubscriberQos*)qos)->group_data.value, qos_list->group_data.value, MAX_SIZE_OF_GROUP_DATA_QOS_POLICY);
			((SubscriberQos*)qos)->presentation.access_scope	= qos_list->presentation.access_scope;
			((SubscriberQos*)qos)->presentation.coherent_access	= qos_list->presentation.coherent_access;
			((SubscriberQos*)qos)->presentation.ordered_access	= qos_list->presentation.ordered_access;
			result	= RETCODE_OK;
			return result;
		}

		/*
		if(!is_enabled || default_datareader_qos == NULL)
		{
			((SubscriberQos*)qos)->entity_factory = qos_list->entity_factory;
			((SubscriberQos*)qos)->group_data	 = qos_list->group_data;
			((SubscriberQos*)qos)->partition		 = qos_list->partition;
			((SubscriberQos*)qos)->presentation	 = qos_list->presentation;
			result		= RETCODE_OK;
			return	result;
		}
		else
		{
			((SubscriberQos*)qos)->entity_factory = qos_list->entity_factory;
			((SubscriberQos*)qos)->group_data	 = qos_list->group_data;

			// Immutable Qos in SubscriberQos --> PRESENTATION
			// inconsistent Qos in SubscriberQos --> PRESENTATION

			// �׳� �ѹ��� Ȯ���� ����� ���°ǰ�? , �׸��� Inconsistent�� �ؾߵǳ� Immutable�� �ؾ� �ϳ� �ָ��ϴ�.... ���� ����
			if(((SubscriberQos*)qos)->presentation.access_scope != qos_list->presentation.access_scope ||
					((SubscriberQos*)qos)->presentation.coherent_access != qos_list->presentation.coherent_access ||
					((SubscriberQos*)qos)->presentation.ordered_access != qos_list->presentation.ordered_access)// ||
				//		((SubscriberQos*)qos)->presentation.name != qos_list->presentation.name)
			{

				result			= RETCODE_IMMUTABLE_POLICY;
				return		result;
			}

			result		= RETCODE_OK;
			return	result;
		}
*/
		result		= RETCODE_OK;
		return	result;
	}

	Listener*			Subscriber::get_listener(void)
	{
		Listener*	result;

		if(!is_enabled)
		{
			result		= NULL;
			return	result;
		}

		result		= listener;
		return	result;
	}

	ReturnCode_t		Subscriber::set_listener(Listener* a_listener, StatusMask mask)
	{
		ReturnCode_t	result;

		if(!is_enabled)
		{
			result	= RETCODE_NOT_ENABLED;
		}

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

	ReturnCode_t		Subscriber::enable(void)
	{
		ReturnCode_t	result;

		is_enabled 		= true;
		result				= RETCODE_OK;

		return	result;
	}

	pDataReader			Subscriber::create_datareader(pTopicDescription a_topic, DataReaderQos* qos, pDataReaderListener a_listener, StatusMask mask)
	{
		DataReader*		result;
		Participant*	rtps_part	= related_participant->related_rtps_participant;
		StatefulReader*	rtps_reader;
		GUID_t			guid		= {0, };

		if(!is_enabled)
		{
			result			= NULL;
			printf("Subscriber::create_dr() Subscriber is not enabled\n");
			return		result;
		}

		if(a_topic != NULL)
		{
			/*
			if(a_topic->get_participant() != this->get_participant()) // a_topic�� participant�� �ش� participant�� ������ Ȯ��
			{
				result 	= NULL;
				printf("Subscriber::create_dr() Topic has some problems\n");
				return	result;
			}
			*/
		}
		else
		{
			result		= NULL;
			printf("Subscriber::create_dr() Topic is NULL\n");
			return	result;
		}

		if(qos == NULL)
		{
			qos	= new DataReaderQos();
			if(get_default_datareader_qos(qos) != RETCODE_OK)
			{
				printf("Subscriber::create_dr() get_default_datareader_qos() has some problems\n");
				return NULL;
			}
		}

		// DataReader ����,�� ����
		result		= new DataReader();

		result->set_qos(qos);
		result->set_listener(a_listener, mask);
		result->related_subscriber	= this;
		result->related_participant	= related_participant;
		result->related_topic_description = a_topic;

		if( (qos != NULL) && ((SubscriberQos*)qos)->entity_factory.auto_enable_created_entities )
		{
			result->enable();
		}

		// DataReaderList�� DataReader �߰�
		datareader_list->insertInRear(result);

		guid.guid_prefix					= related_participant->related_rtps_participant->guid.guid_prefix;
		guid.entity_id.entity_key[0]		= ++entity_key;
		guid.entity_id.entity_kind			= ENTITY_KIND_USER_DEFINED_READER_WITH_KEY;
		rtps_reader							= new StatefulReader(guid, RELIABILITY_KIND_RELIABLE);
		rtps_reader->related_dcps_reader	= result;
		result->related_rtps_reader			= (Reader*)rtps_reader;

		related_participant->_datareader_is_created(result);
		rtps_reader->start();

		return	result;
	}

	void				Subscriber::add_datareader(pDataReader reader)
	{
		datareader_list->insertInRear(reader);
	}

	ReturnCode_t		Subscriber::delete_datareader(pDataReader a_datareader)
	{
		ReturnCode_t	result;

		// �˻��ϴ� a_datareader�� NULL�̸� �ƹ��ų� ��ȯ���� �ƴϸ� ���� ó������ Ȯ�� �غ��� �� (���� ����) --> ���忡 ��� �ȵǾ� ���� �켱 �ּ� ó��
		/*
		if(a_datareader == NULL)
		{
			result		= RETCODE_PRECONDITION_NOT_MET;
			return	result;
		}
		*/

		// �ش� DataReader�� Subscriber�� ���ԵǴ��� Ȯ��
		if(a_datareader->get_subscriber() != this)
		{
			result		= RETCODE_PRECONDITION_NOT_MET;
			return	result;
		}

		// �ش� DataReader�� ReadCondition & QueryCondition�� �����ϰ� �ִ��� Ȯ��
		if(a_datareader->read_condition_list.getSize() != 0)
		{
			result		= RETCODE_PRECONDITION_NOT_MET;
			return	result;
		}

		// �ش� DataReader�� read �Ǵ� take�� �����ϰ� �ִ��� Ȯ�� (���� �ʿ�) --> DataReader �����ϸ鼭 �����ؾ� �� �κ����� ���
		if(a_datareader->read_flag == true)
		{
			result		= RETCODE_PRECONDITION_NOT_MET;
			return	result;
		}

		int								 i	= 0;
		Node<DataReader>*	node	= datareader_list->getNodeByIndex(0);

		while(datareader_list->getSize() > i++)
		{
			node	= node->rear;
			if(node->value == a_datareader) // ������ ��带 ã�� ��� ��� ���� �� while�� break
			{
				datareader_list->cutNode(node);
				delete(node->value);
				delete(node);
				result		= RETCODE_OK;
				break;
			}
		}

		return	result;
	}

	pDataReader			Subscriber::lookup_datareader(std::string topic_name) // �ټ��� DataReader�� �����ϴ� ��� �ƹ��ų� ����
	{
		DataReader*	result;

		int								 i 	= 0;
		Node<DataReader>*	node	= datareader_list->getNodeByIndex(0);

		while(datareader_list->getSize() > i++)
		{
			node	= node->rear;
			if(!node->value->get_topic_description()->get_topic_name().compare(topic_name)) // topic�� ��ġ�ϴ� ��� topic�� ������ DataReader�� ��ȯ
			{
				result 	= node->value;
				return	result;
			}
		}

		// ���� �� ã�����Ƿ� NULL�� ��ȯ
		result		= NULL;
		return	result;
	}

	// ���� �̿Ϸ�
	ReturnCode_t		Subscriber::begin_access(void)
	{
		ReturnCode_t	result;

		// ������ ȣ���� begin_access ���Ŀ� end_access�� ȣ��Ǿ����� Ȯ���� ����� �ʿ� (���� �ʿ�)
		// ReTiCom������ Flag�� �̿�����

		// Subscriber�� Presentation Qos�� access_scope == 'GROUP'���� Ȯ��
		if(((SubscriberQos*)qos)->presentation.access_scope != GROUP_PRESENTATION_QOS)
		{
			result		= RETCODE_PRECONDITION_NOT_MET;
			return	result;
		}

		result		= RETCODE_OK;
		return	result;
	}

	// ���� �̿Ϸ�
	ReturnCode_t		Subscriber::end_access(void)
	{
		ReturnCode_t	result;

		// Subscriber�� Presentation Qos�� access_scope == 'GROUP'���� Ȯ��
		if(((SubscriberQos*)qos)->presentation.access_scope != GROUP_PRESENTATION_QOS)
		{
			result		= RETCODE_PRECONDITION_NOT_MET;
			return	result;
		}

		result		= RETCODE_OK;
		return	result;
	}

	pDomainParticipant	Subscriber::get_participant(void)
	{
		DomainParticipant*	result;

		result		= related_participant;

		return	result;
	}

	// ���� �̿Ϸ�
	// QoS�� ���� ����, 'set'�� 'list'�� ������ �ľ�, ordered_access�� true�� ��� ��� �����Ͽ� list�� ������ ����
	// 'set'�� �ߺ��� ����, 'list'�� �ߺ� ����
	// ������ ������ ������ �ǹ�?

	ReturnCode_t		Subscriber::get_datareaders(DomainEntityList* readers, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states)
	{
		ReturnCode_t	result;

		// DataReader�� ������ Subscriber�� Presentation QoS Policy�� Group�� ��� begin/end_access�� ȣ�� �ؾ� ��

		if (((SubscriberQos*)qos)==NULL)
		{
			result		= RETCODE_PRECONDITION_NOT_MET;
			return	result;
		}

		if( ((SubscriberQos*)qos)->presentation.access_scope == GROUP_PRESENTATION_QOS)
		{
			if(begin_access() != RETCODE_OK)
			{
				result		= RETCODE_PRECONDITION_NOT_MET;
				return 	result;
			}

			if( ((SubscriberQos*)qos)->presentation.ordered_access != TRUE) // Not Specified Ordered (Maybe 'set')
			{
				int		i	= 0;
				int		j	= 0;

				Node<DataReader>*	node	= datareader_list->getNodeByIndex(0);

				while(datareader_list->getSize() > i++) // State�� �����ϴ� Reader�� ã�Ƽ� list�� ����
				{
					node	= node->rear; // DataReader Point

					Node<DataSample>* state_node = node->value->data_sample_list.getNodeByIndex(0);

					while(node->value->data_sample_list.getSize() > j++)
					{
						state_node = state_node->rear; // DataReader�� DataSample�� ����

						// Parameter�� �Ѿ�� 3���� ������ ��� �����ϴ� DataReader�� ��� readers List�� �ش� DataReader�� �߰�
						if((state_node->value->info->sample_state & sample_states) &&
								(state_node->value->info->view_state & view_states) &&
								(state_node->value->info->instance_state & instance_states))
						{
							// ���� ��� ������ readers list�� �߰�
							readers->insertInRear(node->value);
							break;
						}
					}
				}
			}
			else // Specified Ordered - �̱��� (Maybe 'list')
			{
				int		i	= 0;
				int		j	= 0;

				Node<DataReader>*node	= datareader_list->getNodeByIndex(0);

				while(datareader_list->getSize() > i++)
				{
					node	=	node->rear;

					Node<DataSample>* state_node		= node->value->data_sample_list.getNodeByIndex(0);

					while(node->value->data_sample_list.getSize() > j++)
					{
						state_node = state_node->rear;

					}
				}
			}

			if(end_access() != RETCODE_OK)
			{
				result		= RETCODE_PRECONDITION_NOT_MET;
				return	result;
			}

			result		= RETCODE_OK;
		}
		else // Presentation Qos�� GROUP�� �ƴ� ��� (TOPIC, INSTANCE)  -- 'set'
		{
			int		i	= 0;
			int		j	= 0;

			Node<DataReader>*	node	= datareader_list->getNodeByIndex(0);

			while(datareader_list->getSize() > i++) // State�� �����ϴ� Reader�� ã�Ƽ� list�� ����
			{
				node	= node->rear; // DataReader Point

				Node<DataSample>* state_node = node->value->data_sample_list.getNodeByIndex(0);

				while(node->value->data_sample_list.getSize() > j++)
				{
					state_node = state_node->rear; // DataReader�� DataSample�� ����

					// Parameter�� �Ѿ�� 3���� ������ ��� �����ϴ� DataReader�� ��� readers List�� �ش� DataReader�� �߰�
					// �߰� �ϰ� �ش� DataReader�� �ߺ��ؼ� �߰��ϸ� �ȵǹǷ� break
					if((state_node->value->info->sample_state & sample_states) &&
							(state_node->value->info->view_state & view_states) &&
							(state_node->value->info->instance_state & instance_states))
					{
						// ���� ��� ������ readers list�� �߰�
						readers->insertInRear(node->value);
						break;
					}
				}
			}
		}

		result		= RETCODE_OK;
		return	result;
	}

	ReturnCode_t		Subscriber::notify_datareaders(void)
	{
		ReturnCode_t	result;
		// �����Ͱ� Available�� ������ DataReader�� �� Listener�� ��ϵǾ� �ִ� Reader�鸸 DataReaderListener �Լ��� on_data_available�� ��� ȣ���Ѵ�.

		int							i	= 0;
		Node<DataReader>* node 	= datareader_list->getNodeByIndex(0);
		DataReaderListener* pListener;

		while(datareader_list->getSize() > i++)
		{
			node	= node->rear;

			// DataReaderListener�� ��ϵǾ� ������ ���°� DATA_AVAILABLE�� DataReader�� on_data_available�� ȣ��
			if( (node->value->listener != NULL) && (node->value->get_status_changes() & DATA_AVAILABLE_STATUS))
			{
				pListener = (DataReaderListener*)node->value->get_listener();
				pListener->on_data_available(node->value);
			}
		}

		// ����ó���� ���õ� �κ� ��� ����
		result		= RETCODE_OK;
		return	result;
	}

	// ������ ��� Entity�� ����
	ReturnCode_t		Subscriber::delete_contained_entities(void)
	{
		ReturnCode_t	result;

		int									i	= 0;
		Node<DataReader>	*node	= datareader_list->getNodeByIndex(0);

		while(datareader_list->getSize() > i++)
		{
			node = node->rear;

			if(delete_datareader(node->value) != RETCODE_OK) // �ش� Reader�� Delete, �����ϴ� ��� �����ڵ� ��ȯ (DataReader�� Read,QueryCondition�� �����ϴ� ��)
			{
				result = RETCODE_PRECONDITION_NOT_MET;
				return	result;
			}
		}

		result		= RETCODE_OK;
		return	result;
	}

	// ���� �̿Ϸ�
	ReturnCode_t		Subscriber::set_default_datareader_qos(DataReaderQos* qos_list)
	{
		ReturnCode_t	result;

		// Check self consisent??

		default_datareader_qos = qos_list;

		result		= RETCODE_OK;

		return	result;
	}

	ReturnCode_t		Subscriber::get_default_datareader_qos(DataReaderQos* qos_list)
	{
		ReturnCode_t	result;

		memcpy(qos_list, default_datareader_qos, sizeof(DataReaderQos));

		result		= RETCODE_OK;

		return	result;
	}

	// DataReader Qos�� Topic Qos�� ��ġ�ϴ� �κ��� copy
	ReturnCode_t		Subscriber::copy_from_topic_qos(DataReaderQos* a_datareader_qos, TopicQos* a_topic_qos)
	{
		ReturnCode_t	result;

		// deadline, destination_order, durability, history, latency_budget, liveliness, ownership, reliability, resource_limits

		if(a_datareader_qos != NULL && a_topic_qos != NULL) // QoS�� NULL�� ��쿡 ��� �����ϴ��� SPEC��� ���� (���� ����)
		{
			a_datareader_qos->deadline = a_topic_qos->deadline;
			a_datareader_qos->destination_order = a_topic_qos->destination_order;
			a_datareader_qos->durability = a_topic_qos->durability;
			a_datareader_qos->history = a_topic_qos->history;
			a_datareader_qos->latency_budget = a_topic_qos->latency_budget;
			a_datareader_qos->liveliness = a_topic_qos->liveliness;
			a_datareader_qos->ownership = a_topic_qos->ownership;
			a_datareader_qos->reliability = a_topic_qos->reliability;
			a_datareader_qos->resource_limits = a_topic_qos->resource_limits;
			result		= RETCODE_OK;
		}
		else
		{
			result		= RETCODE_PRECONDITION_NOT_MET;
		}

		return	result;
	}

	void				Subscriber::_set_default_datareader_qos(void)
	{
		this->default_datareader_qos	= new DataReaderQos();

		this->default_datareader_qos->deadline.period.sec		= 0;
		this->default_datareader_qos->deadline.period.nanosec	= 0;

		this->default_datareader_qos->destination_order.kind	= BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;

		this->default_datareader_qos->durability.kind	= VOLATILE_DURABILITY_QOS;

		this->default_datareader_qos->history.kind	= KEEP_ALL_HISTORY_QOS;
		this->default_datareader_qos->history.depth	= -1;

		this->default_datareader_qos->latency_budget.duration.sec		= 0;
		this->default_datareader_qos->latency_budget.duration.nanosec	= 0;

		this->default_datareader_qos->liveliness.kind					= AUTOMATIC_LIVELINESS_QOS;
		this->default_datareader_qos->liveliness.lease_duration.sec		= 0;
		this->default_datareader_qos->liveliness.lease_duration.nanosec	= 0;

		this->default_datareader_qos->ownership.kind	= SHARED_OWNERSHIP_QOS;

		this->default_datareader_qos->reader_data_lifecycle.auto_purge_disposed_samples_delay.sec		= 30;
		this->default_datareader_qos->reader_data_lifecycle.auto_purge_disposed_samples_delay.nanosec	= 0;
		this->default_datareader_qos->reader_data_lifecycle.auto_purge_no_writer_samples_delay.sec		= 30;
		this->default_datareader_qos->reader_data_lifecycle.auto_purge_no_writer_samples_delay.nanosec	= 0;

		this->default_datareader_qos->reliability.kind	= RELIABLE_RELIABILITY_QOS;
		this->default_datareader_qos->reliability.max_blocking_time.sec		= 0;
		this->default_datareader_qos->reliability.max_blocking_time.nanosec	= 0;

		this->default_datareader_qos->resource_limits.max_instances				= -1;
		this->default_datareader_qos->resource_limits.max_samples				= -1;
		this->default_datareader_qos->resource_limits.max_samples_per_instance	= -1;

		this->default_datareader_qos->time_based_filter.minimum_separation.sec		= 0;
		this->default_datareader_qos->time_based_filter.minimum_separation.nanosec	= 0;

		memset(this->default_datareader_qos->user_data.value, 0, MAX_SIZE_OF_USER_DATA_QOS_POLICY);
	}
}
