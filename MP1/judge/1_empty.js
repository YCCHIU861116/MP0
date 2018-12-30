 // 測資一：空目錄

const child_process = require("child_process")
const constant = require("./constant.js");

const directory = "1_empty"

module.exports = {
	description: "測資一：空目錄",
	directory: directory,
	create_data: function () {
		child_process.execSync(`mkdir -p data/${directory}`);
	},
	status: {
		answer: constant.STATUS_NOTHING_CHANGE,
		alias: "status"
	},
	commit: {
		not_exist: true,
		alias: "commit"
	},
	log: {
		number: 1,
		answer: "",
		alias: "log"
	}
}
