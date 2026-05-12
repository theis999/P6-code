using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.Authentication;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc.RazorPages;
using System.Net.Http;
using System.Security.Claims;
using System;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;


namespace newnewWebinterface.Pages;

[Authorize]
public class RegisterModel : PageModel
{
    public string name;
    public string ProductID;

    public async Task<IActionResult> OnPostAsync()
    {
        string authentikUserID = User.FindFirstValue("sub");

        using StringContent jsonContent = new(
         JsonSerializer.Serialize(new
         {
             name = name,
             ProductID = ProductID,
             authentikUserID = authentikUserID
         }),
        System.Text.Encoding.UTF8,
        "application/json");

        var client = new HttpClient();
        using HttpResponseMessage response = await client.PostAsync("https://smakdb.head9x.dk/boards", jsonContent);

        System.Diagnostics.Debug.WriteLine("This will be displayed in output window");

        return RedirectToPage("./Index");
    }
}
