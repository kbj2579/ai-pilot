#ifndef ABSTRACT_LOGGER_H
#define ABSTRACT_LOGGER_H

#include "behaviortree_cpp_v3/behavior_tree.h"
#include "behaviortree_cpp_v3/bt_factory.h"

namespace BT
{
	enum TimestampType
	{
		ABSOLUTE,
		RELATIVE,
	};

	typedef std::array<uint8_t, 12> SerializedTransition;

	class StatusChangeLogger
	{
	public:
		StatusChangeLogger(BT::TreeNode *root_node);

		virtual ~StatusChangeLogger() = default;

		virtual void callback(BT::Duration timestamp, const BT::TreeNode& node, BT::NodeStatus prev_status, BT::NodeStatus status) = 0;

		virtual void flush() = 0;

		void setEnabled(bool enabled);
		/*{
			enabled_ = enabled;
		}*/

		void seTimestampType(enum TimestampType type);
		/*{
			type_ = type;
		}*/

		bool enabled() const;
		/*{
			return enabled_;
		}*/

		// false by default.
		bool showsTransitionToIdle() const;
		/*{
			return show_transition_to_idle_;
		}*/

		void enableTransitionToIdle(bool enable);
		/*{
			show_transition_to_idle_ = enable;
		}*/

	private:
		bool enabled_;
		bool show_transition_to_idle_;
		std::vector<BT::TreeNode::StatusChangeSubscriber> subscribers_;
		enum TimestampType type_;
		BT::TimePoint first_timestamp_;
	};

	//--------------------------------------------

	inline void StatusChangeLogger::setEnabled(bool enabled)
	{
		enabled_ = enabled;
	}

	inline void StatusChangeLogger::seTimestampType(enum TimestampType type)
	{
		type_ = type;
	}

	inline bool StatusChangeLogger::enabled() const
	{
		return enabled_;
	}

	// false by default.
	inline bool StatusChangeLogger::showsTransitionToIdle() const
	{
		return show_transition_to_idle_;
	}

	inline void StatusChangeLogger::enableTransitionToIdle(bool enable)
	{
		show_transition_to_idle_ = enable;
	}

	StatusChangeLogger::StatusChangeLogger(BT::TreeNode* root_node) : enabled_(true), show_transition_to_idle_(true), type_(BT::TimestampType::ABSOLUTE)
	{
		first_timestamp_ = std::chrono::high_resolution_clock::now();

		auto subscribeCallback = [this](BT::TimePoint timestamp, const BT::TreeNode& node, BT::NodeStatus prev,	BT::NodeStatus status) 
		{
			if (enabled_ && (status != BT::NodeStatus::IDLE || show_transition_to_idle_))
			{
				if (type_ == ABSOLUTE)
				{
					this->callback(timestamp.time_since_epoch(), node, prev, status);
				}
				else
				{
					this->callback(timestamp - first_timestamp_, node, prev, status);
				}
			}
		};

		auto visitor = [this, subscribeCallback](BT::TreeNode* node) {
			subscribers_.push_back(node->subscribeToStatusChange(std::move(subscribeCallback)));
		};

		applyRecursiveVisitor(root_node, visitor);
	}
}

#endif   // ABSTRACT_LOGGER_H
