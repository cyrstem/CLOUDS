function Clip(_class, _x, _y, _fill) {
	this.id = _class;
	this.originalX = _x;
	this.originalY = _y;
	this.originalColor = _fill; //original color. just using it as input data. 
	this.visited = false;
	this.neighbors = new Array();
	this.person = null;
}

Clip.prototype.markAsVisited = function() {
	this.visited = true;
}

Clip.prototype.addNeighbor = function(neighborClip){
	this.neighbors.push(neighborClip);
}

Clip.prototype.setPerson = function(_person){
	this.person = _person;
}





