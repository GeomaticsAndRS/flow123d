/*
 /**!
 *
 * Copyright (C) 2007 Technical University of Liberec.  All rights reserved.
 *
 * Please make a following refer to Flow123d on your project site if you use the program for any purpose,
 * especially for academic research:
 * Flow123d, Research Centre: Advanced Remedial Technologies, Technical University of Liberec, Czech Republic
 *
 * This program is free software; you can redistribute it and/or modify it under the terms
 * of the GNU General Public License version 3 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program; if not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 021110-1307, USA.
 *
 *
 * $Id: bih_node.cc 1567 2012-02-28 13:24:58Z jan.brezina $
 * $Revision: 1567 $
 * $LastChangedBy: jan.brezina $
 * $LastChangedDate: 2012-02-28 14:24:58 +0100 (Tue, 28 Feb 2012) $
 *
 *
 */

#include "new_mesh/bih_node.hh"
#include "new_mesh/bounding_box.hh"

BIHNode::BIHNode(arma::vec3 minCoordinates, arma::vec3 maxCoordinates, int splitCoor, int depth) : BoundingIntevalHierachy() {
	//xprintf(Msg, " - BIHNode->BIHNode(arma::vec3, arma::vec3, int, int)\n");

	leaf_ = false;
	boundingBox_ = new BoundingBox(minCoordinates, maxCoordinates);
	splitCoor_ = splitCoor;
	depth_ = depth;
}

void BIHNode::put_element(int element_id) {
	element_ids_.push_back(element_id); //TODO: element_id can be add only once
}

double BIHNode::get_median_coord(std::vector<BoundingBox *> elements, int index) {
	int boundingBoxIndex = *(element_ids_.begin() + index);
	return elements[boundingBoxIndex]->get_center()(splitCoor_);
}

void BIHNode::distribute_elements(std::vector<BoundingBox *> elements) {
	for (std::vector<int>::iterator it = element_ids_.begin(); it!=element_ids_.end(); it++) {
		int id = *it;
		BoundingBox* boundingBox = (*(elements.begin() + id));
		for (int j=0; j<child_count; j++) {
			if (child_[j]->contains_element(splitCoor_, boundingBox->get_min()(splitCoor_), boundingBox->get_max()(splitCoor_))) {
				((BIHNode *)child_[j])->put_element(id);
			}
		}
	}

	element_ids_.erase(element_ids_.begin(), element_ids_.end());

	((BIHNode *)child_[0])->split_distribute(elements);
	((BIHNode *)child_[1])->split_distribute(elements);
}

void BIHNode::find_elements(BoundingBox &boundingBox, std::vector<int> &searchedElements, std::vector<BoundingBox *> meshElements) {
	if (leaf_) {
		for (std::vector<int>::iterator it = element_ids_.begin(); it!=element_ids_.end(); it++) {
			int id = *it;
			BoundingBox* b = (*(meshElements.begin() + id));;

			if (b->intersection(boundingBox)) {
				searchedElements.push_back(id);
			}
		}
	} else {
		if (child_[0]->boundingBox_->intersection(boundingBox)) ((BIHNode *)child_[0])->find_elements(boundingBox, searchedElements, meshElements);
		if (child_[1]->boundingBox_->intersection(boundingBox)) ((BIHNode *)child_[1])->find_elements(boundingBox, searchedElements, meshElements);
	}
}

int BIHNode::get_element_count() {
	return element_ids_.size();
}
