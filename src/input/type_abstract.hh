/*!
 *
﻿ * Copyright (C) 2015 Technical University of Liberec.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 3 as published by the
 * Free Software Foundation. (http://www.gnu.org/licenses/gpl-3.0.en.html)
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 *
 * @file    type_abstract.hh
 * @brief
 */

#ifndef TYPE_ABSTRACT_HH_
#define TYPE_ABSTRACT_HH_

#include "system/exceptions.hh"

#include "type_base.hh"
#include "type_selection.hh"


namespace Input {
namespace Type {



/**
 * @brief Class for declaration of polymorphic Record.
 *
 * Like the Record class this is only proxy class. It allows add Record as descendants, but
 * further there is method \p no_more_descendants to close adding them. After this
 * you can not derive any Record from this Abstract.
 *
 *
 *
 * A static method (typically part of an abstract class) for construction of an AbstractType can look like:
 *
 @code
    static Input::Type::Abstract &SomeAbstractClass::get_input_type() {
        using namespace Input::Type;
        return Abstract("Function", "Input for generic time-space function.")
        			.close();
    }
 @endcode
 *
 * @ingroup input_types
 */
class Abstract : public TypeBase {
	friend class OutputBase;
	//friend class Record;
	friend class AdHocAbstract;

protected:

    /**
     * @brief Actual data of the abstract record.
     */
    class ChildData {
    public:
    	/// Constructor
        ChildData(const string &name, const string &description)
        : selection_of_childs( std::make_shared<Selection> (name + "_TYPE_selection") ),
		  description_(description),
		  type_name_(name),
		  finish_status_(FinishStatus::none_),
		  closed_(false),
		  selection_default_(Default::obligatory())
        {}

        /**
         * @brief Selection composed from names of derived Records.
         *
         * Indices are according to the order of derivation (starting from zero).
         */
        std::shared_ptr< Selection> selection_of_childs;

        /// Vector of derived Records (proxies) in order of derivation.
        vector< Record > list_of_childs;

        /// Description of the whole Abstract type.
        const string description_;

        /// type_name of the whole Abstract type.
        const string type_name_;

        /// Abstract is finished when it has added all descendant records.
        FinishStatus finish_status_;

        /// If Abstract is closed, we do not allow any further declaration calls.
        bool closed_;

        /**
         * @brief Default value of selection_of_childs (used for automatic conversion).
         *
         * If default value isn't set, selection_default_ is set to obligatory.
         */
        Default selection_default_;

        /**
         * Allow store hash of part of generic subtree.
         *
         * This hash can be typically used if descendants of Abstract contains different
         * structure of parameter location.
         * For example we have have Records with key represents generic part of subtree:
         *  - in first descendant this key is of the type Parameter
         *  - in second descendant this key is of the type Array of Parameter
         *  - in third descendant this key is of the type Array of Parameter with fixed size
         *  etc.
         */
        //TypeHash generic_content_hash_;

    };

public:
    /// Public typedef of constant iterator into array of keys.
    typedef std::vector< Record >::const_iterator ChildDataIter;

    /// Default constructor.
    Abstract();

    /**
     * @brief Copy constructor.
     *
     * We check that other is non empty.
     */
    Abstract(const Abstract& other);


    /**
     * @brief Basic constructor.
     *
     * You has to provide \p type_name of the new declared Abstract type and its \p description.
     */
    Abstract(const string & type_name_in, const string & description);

    /**
     * @brief Implements @p TypeBase::content_hash.
     *
     * Hash is calculated by type name, description and hash of attributes.
     */
    TypeHash content_hash() const   override;

    /**
     * @brief Allows shorter input of the Abstract providing the default value to the "TYPE" key.
     *
     * If the input reader come across the Abstract in the declaration tree and the input
     * is not 'record-like' with specified value for TYPE, it tries to use the descendant Record specified by
     * @p type_default parameter of this method. Further auto conversion of such Record may be possible.
     */
    Abstract &allow_auto_conversion(const string &type_default);

    /// Can be used to close the Abstract for further declarations of keys.
    Abstract &close();

    /**
     *  Mark the type to be root of a generic subtree.
     *  Such type can not appear in IST directly but only as the internal type
     *  of the Instance auxiliary object.
     */
    Abstract &root_of_generic_subtree();

    /// Frontend to TypeBase::add_attribute_
    Abstract &add_attribute(std::string key, TypeBase::json_string value);

    /**
     *  @brief Finish declaration of the Abstract type.
     *
     *  Set Abstract as parent of derived Records (for mechanism of
     *  set parent and descendant see \p Record::derive_from)
     */
    FinishStatus finish(FinishStatus finish_type = FinishStatus::regular_) override;

    /// Returns reference to the inherited Record with given name.
    const Record  &get_descendant(const string& name) const;

    /**
     * @brief Returns default descendant.
     *
     * Returns only if TYPE key has default value, otherwise returns empty Record.
     */
    const Record * get_default_descendant() const;

    /// Returns reference to Selection type of the implicit key TYPE.
    const Selection &get_type_selection() const;

    /// Returns number of descendants in the child_data_.
    unsigned int child_size() const;

    /// Implements @p TypeBase::finish_status.
    virtual FinishStatus finish_status() const override;

    /// Implements @p TypeBase::is_finished.
    virtual bool is_finished() const override;

    /// Returns true if @p data_ is closed.
    virtual bool is_closed() const override;

    /**
     * @brief Implements @p Type::TypeBase::type_name.
     *
     * Name corresponds to @p data_->type_name_.
     */
    virtual string type_name() const override;
    /// Override @p Type::TypeBase::class_name.
    string class_name() const override { return "Abstract"; }

    /**
     * @brief Container-like access to the descendants of the Abstract.
     *
     * Returns iterator to the first data.
     */
    ChildDataIter begin_child_data() const;

    /**
     * @brief Container-like access to the descendants of the Abstract.
     *
     * Returns iterator to the last data.
     */
    ChildDataIter end_child_data() const;

    /**
     * @brief Add inherited Record.
     *
     * This method is used primarily in combination with registration variable. @see Input::Factory
     *
     * Example of usage:
	 @code
		 class SomeBase
		 {
		 public:
    		/// the specification of input abstract record
    		static const Input::Type::Abstract & get_input_type();
			...
		 }

		 class SomeDescendant : public SomeBase
		 {
		 public:
    		/// the specification of input record
    		static const Input::Type::Record & get_input_type();
			...
		 private:
			/// registers class to factory
			static const int reg;
		 }

		 /// implementation of registration variable
		 const int SomeDescendant::reg =
			 Input::register_class< SomeDescendant >("SomeDescendant") +
			 SomeBase::get_input_type().add_child(SomeDescendant::get_input_type());
	 @endcode
     */
    int add_child(const Record &subrec);

    // Get default value of selection_of_childs
    Default &get_selection_default() const;

    // Implements @p TypeBase::make_instance.
    virtual MakeInstanceReturnType make_instance(std::vector<ParameterPair> vec = std::vector<ParameterPair>()) override;




protected:
    /// Create deep copy of Abstract (copy all data stored in shared pointers etc.)
    Abstract deep_copy() const;

    /// Check if type has set value of default descendants.
    bool have_default_descendant() const;

    /// Actual data of the Abstract.
    std::shared_ptr<ChildData> child_data_;

    friend class Record;
};


/** ******************************************************************************************************************************
 * @brief Class for declaration of polymorphic Record.
 *
 * Abstract extends on list of descendants provided immediately
 * after construction by add_child(). These descendants derive
 * only keys from common AR. AdHocAR has separate instance for every
 * key of this type.
 *
 * @ingroup input_types
 */
class AdHocAbstract : public Abstract {
	friend class OutputBase;
public:
	/// Constructor
	AdHocAbstract(const Abstract &ancestor);

    /**
     * @brief Implements @p TypeBase::content_hash.
     *
     * Hash is calculated by type name, description and name of ancestor.
     */
	TypeHash content_hash() const   override;

    /// Override @p Type::TypeBase::class_name.
	string class_name() const override { return "AdHocAbstract"; }


    /**
     * @brief Finish declaration of the AdHocAbstract type.
     *
     * Adds descendants of ancestor Abstract, calls close() and complete keys with non-null
     * pointers to lazy types.
     */
	FinishStatus finish(FinishStatus finish_type = FinishStatus::regular_) override;

    /// Add inherited Record.
    AdHocAbstract &add_child(const Record &subrec);

protected:
    /// Reference to ancestor Abstract
    const Abstract &ancestor_;
};



} // closing namespace Type
} // closing namespace Input




#endif /* TYPE_ABSTRACT_HH_ */
