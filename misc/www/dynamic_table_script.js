function addRow() {
    const nameInput = document.getElementById('nameInput').value;
    const ageInput = document.getElementById('ageInput').value;

    if (nameInput === "" || ageInput === "") {
        alert("Please fill out both fields");
        return;
    }

    const table
= document.getElementById('dynamicTable').getElementsByTagName('tbody')[0];
    const newRow = table.insertRow();

    const nameCell = newRow.insertCell(0);
    const ageCell = newRow.insertCell(1);

    nameCell.textContent = nameInput;
    ageCell.textContent = ageInput;

    document.getElementById('nameInput').value = "";
    document.getElementById('ageInput').value = "";
}

